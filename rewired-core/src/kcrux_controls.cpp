#include "kcrux_controls.hpp"
#include "kcrux_input_state.hpp"
#include "kcrux_slots.hpp"
#include "service_imports.hpp"
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_mg_rod.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "d/d_menu_ring.h"
#include "d/d_menu_item_explain.h"
#include "d/d_msg_flow.h"
#include "m_Do/m_Do_controller_pad.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "mods/svc/hook.hpp"
#include <algorithm>
#include <vector>
#include <chrono>
#include <cstdio>

// Public host symbols; this private header is not included in the mod SDK.
namespace dusk {
enum class ActionBinds { FIRST_PERSON_CAMERA,CALL_MIDNA,OPEN_MAP_SCREEN,TOGGLE_MINIMAP,
    OPEN_DUSKLIGHT_MENU,TURBO_SPEED_BUTTON,COUNT };
}

namespace dawnlight {
namespace {
DEFINE_HOOK(&mDoCPd_c::read,KcruxPadRead);
DEFINE_HOOK(&daAlink_c::setStickData,KcruxStickData);
DEFINE_HOOK(&dComIfGs_getSelectItemIndex,KcruxSelectIndex);
DEFINE_HOOK(&dComIfGs_getMixItemIndex,KcruxMixIndex);
DEFINE_HOOK(&dComIfGp_getSelectItem,KcruxPlayItem);
DEFINE_HOOK(&dComIfGp_setSelectItem,KcruxSetPlayItem);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor,KcruxRingAssign);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOn,RewiredMixOn);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOff,RewiredMixOff);
DEFINE_HOOK(&dComIfGs_setSelectItemIndex,RewiredCollectionAssign);
DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton,KcruxStow);
DEFINE_HOOK(&daAlink_c::checkItemSetButton,KcruxItemSetButton);
DEFINE_HOOK(&daAlink_c::checkNewItemChange,KcruxNewItemChange);
DEFINE_HOOK(&daAlink_c::procFishingFoodInit,KcruxFishingFoodInit);
DEFINE_HOOK(&dMsgFlow_c::query021,KcruxEquippedItemQuery);
DEFINE_HOOK(&daAlink_c::orderTalk,KcruxTradeTalk);
DEFINE_HOOK_SYMBOL("_ZL15dmg_rod_ExecuteP13dmg_rod_class",int(dmg_rod_class*),KcruxRodExecute);
DEFINE_HOOK(&daAlink_c::setHorseGetOff,KcruxDismount);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger,RewiredMidnaTrigger);
DEFINE_HOOK(&daAlink_c::wallGrabTrigger,KcruxGrabTrigger);
DEFINE_HOOK(&daAlink_c::wallGrabButton,KcruxGrabHold);
DEFINE_HOOK(&daAlink_c::spActionTrigger,KcruxSecondaryTrigger);
DEFINE_HOOK(&daAlink_c::spActionButton,KcruxSecondaryHold);
DEFINE_HOOK_SYMBOL("dusk::updateActionBindings",void(),KcruxMapSample);
DEFINE_HOOK_SYMBOL("dusk::getActionBindTrig",bool(dusk::ActionBinds,u32),KcruxMapTrig);
ConfigVarHandle s_enabled=0;
bool s_modeAtBoot=false;
using ButtonFn=bool(*)(SDL_Gamepad*,int);
using AxisFn=short(*)(SDL_Gamepad*,int);
ButtonFn s_button=nullptr;AxisFn s_axis=nullptr;
kcrux::Slots s_slots;
std::array<kcrux::Button,4> s_dpad;
kcrux::Button s_items,s_mapMenu,s_mapWarp;
kcrux::Button s_sword,s_cancel,s_view,s_object,s_midna;
kcrux::Trigger s_fire,s_quick,s_target;
kcrux::Trigger s_menuTrigger;
kcrux::Button s_menuLeft,s_menuRight,s_menuCombine,s_wheelCombine;
u8 s_collectionSelection=255;
bool s_collectionCombine=false;
std::array<kcrux::Button,4> s_menuDpad;
bool s_collectionOpen=false;
int s_collectionPage=0,s_collectionTarget=-1;
kcrux::FireState s_fireState;
kcrux::MapGesture s_mapGesture;
bool s_mapOverride=false;
SaveObserverHandle s_saveObserver=0;
bool s_loaded=false,s_boss=false,s_wolf=false;
int s_active=-1,s_wheelRequest=-1;
unsigned s_revision=0;
daAlink_c* s_owner=nullptr;u32 s_actor=0;
thread_local int s_querySlot=-1;
int s_fishingRodSlot=-1,s_fishingProxy=-1;
daAlink_c* s_fishingOwner=nullptr;
bool s_fishingStarted=false;
int s_tradeSlot=-1,s_tradeFrames=0;
daAlink_c* s_tradeOwner=nullptr;
u8 s_tradeTrigger=0;
bool s_tradePending=false,s_tradeSeenEvent=false;
int s_reelRodSlot=-1;
dmg_rod_class* s_reelActor=nullptr;
u32 s_reelHeld=0;
constexpr int NativeFlags[3]={PAD_BUTTON_X,PAD_BUTTON_Y,PAD_TRIGGER_Z};
bool blocked();
bool ui_visible();
HookAction assign_from_collection(ModContext*,void*,void*,void*){
    // The native three-channel assignment transaction must never mutate the
    // compatibility mirrors while the five-slot menu owns assignment.
    return kcrux_controls_enabled()&&dMeter2Info_getWindowStatus()==3&&s_collectionPage==1
        ?HOOK_SKIP_ORIGINAL:HOOK_CONTINUE;
}
void clear_fishing_projection(bool equipRod);
void sample_map_gesture(ModContext*,void*,void*,void*){
    const auto index=PADGetIndexForPort(PAD_1);auto* pad=index<0?nullptr:PADGetSDLGamepadForIndex(index);
    const bool custom=kcrux_controls_enabled()&&pad;
    // Only gameplay uses the tap/hold split. Native menus keep their controls.
    s_mapOverride=custom&&dMeter2Info_getWindowStatus()==0;
    const double stamp=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    s_mapGesture.sample(custom&&s_button&&s_button(pad,9),!s_mapOverride||blocked(),stamp);
    s_midna.sample(custom&&s_button&&s_button(pad,7),!custom||blocked());
    s_mapMenu.sample(custom&&s_button&&s_button(pad,9),!custom||(dMeter2Info_getWindowStatus()!=4&&dMeter2Info_getWindowStatus()!=5)||ui_visible());
}
void after_midna_trigger(ModContext*,void*,void* result,void*){
    if(kcrux_controls_enabled())*static_cast<BOOL*>(result)=s_midna.pressed;
}
HookAction map_action_trigger(ModContext*,void* args,void* result,void*){
    if(!kcrux_controls_enabled()||mods::arg<u32>(args,1)!=PAD_1)return HOOK_CONTINUE;
    const auto action=mods::arg<dusk::ActionBinds>(args,0);
    if(action==dusk::ActionBinds::CALL_MIDNA){
        *static_cast<bool*>(result)=s_midna.pressed;return HOOK_SKIP_ORIGINAL;
    }
    if(action!=dusk::ActionBinds::OPEN_MAP_SCREEN&&action!=dusk::ActionBinds::TOGGLE_MINIMAP)return HOOK_CONTINUE;
    *static_cast<bool*>(result)=action==dusk::ActionBinds::OPEN_MAP_SCREEN
        ?((dMeter2Info_getWindowStatus()==4||dMeter2Info_getWindowStatus()==5)?s_mapMenu.pressed:s_mapOverride&&s_mapGesture.tap)
        :s_mapOverride&&s_mapGesture.hold;
    return HOOK_SKIP_ORIGINAL;
}
HookAction object_input(void* args,void* result,bool trigger){
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!kcrux_controls_enabled()||!link||blocked())return HOOK_CONTINUE;
    // Only the native push/pull and chain consumers see Y as their R input.
    // RT remains an item input; targeting and ordinary R consumers are untouched.
    const bool down=trigger?s_object.pressed:s_object.held;
    link->mUseButtonFlags|=daAlink_c::BTN_R;
    *static_cast<BOOL*>(result)=down;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_grab_trigger(ModContext*,void* args,void* result,void*){return object_input(args,result,true);}
HookAction before_grab_hold(ModContext*,void* args,void* result,void*){return object_input(args,result,false);}
bool secondary_r_status(u8 status){
    switch(status){
    case BUTTON_STATUS_LOCK:case BUTTON_STATUS_SWITCH:case BUTTON_STATUS_SHIELD_ATTACK:
    case BUTTON_STATUS_DEFEND:case BUTTON_STATUS_COVER:return true;
    default:return false;
    }
}
HookAction secondary_r_input(void* args,void* result,bool trigger){
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!kcrux_controls_enabled()||!link||blocked())return HOOK_CONTINUE;
    const u8 status=dComIfGp_getRStatusForce()?dComIfGp_getRStatusForce():dComIfGp_getRStatus();
    // The boomerang actor may ask for its lock trigger before Link refreshes
    // this frame's R prompt. Its active aiming state is the fallback guard.
    const bool boomerangAim=link->mEquipItem==dItemNo_BOOMERANG_e&&kcrux_aiming_item(link);
    if(!secondary_r_status(status)&&!boomerangAim)return HOOK_CONTINUE;
    // The boomerang lock, bow switch and shield/horse R actions belong to Y.
    // Return Y only for these native consumers; RT keeps item selection/fire.
    link->mUseButtonFlags|=daAlink_c::BTN_R;
    *static_cast<BOOL*>(result)=trigger?s_object.pressed:s_object.held;
    return HOOK_SKIP_ORIGINAL;
}
HookAction before_secondary_trigger(ModContext*,void* args,void* result,void*){return secondary_r_input(args,result,true);}
HookAction before_secondary_hold(ModContext*,void* args,void* result,void*){return secondary_r_input(args,result,false);}
struct ActionScope {daAlink_c* link;u8 held,pressed,status;bool changed;};
std::vector<ActionScope> s_actionScopes;

bool stow_available(daAlink_c* link){
    return link&&!link->checkWolf()&&link->mEquipItem!=dItemNo_NONE_e&&link->mEquipItem!=0&&
        link->mEquipItem!=0x102&&link->checkModeFlg(4)&&!link->checkEquipAnime()&&
        !link->checkBoomerangThrowAnime()&&!link->checkCopyRodThrowAnime()&&!link->checkKandelaarSwingAnime();
}
HookAction before_cancel_action(void* args,bool stow){
    auto* link=mods::arg<daAlink_c*>(args,0);
    const bool change=kcrux_controls_enabled()&&link&&!blocked()&&
        (stow?stow_available(link):dComIfGp_getDoStatus()==BUTTON_STATUS_DISMOUNT);
    s_actionScopes.push_back({link,link?link->mItemButton:u8(0),link?link->mItemTrigger:u8(0),
        dComIfGp_getDoStatus(),change});
    if(change){
        // Remap only the native cancellation consumer. The rest of the frame
        // retains physical A for rolling, sprinting and the separate A jump.
        link->mItemButton=(link->mItemButton&~daAlink_c::BTN_A)|(s_cancel.held?daAlink_c::BTN_A:0);
        link->mItemTrigger=(link->mItemTrigger&~daAlink_c::BTN_A)|(s_cancel.pressed?daAlink_c::BTN_A:0);
        if(stow&&s_cancel.pressed)g_dComIfG_gameInfo.play.mItemInfo.mDoStatus=BUTTON_STATUS_PUT_AWAY;
    }
    return HOOK_CONTINUE;
}
HookAction before_stow(ModContext*,void* args,void*,void*){return before_cancel_action(args,true);}
HookAction before_dismount(ModContext*,void* args,void*,void*){return before_cancel_action(args,false);}
void after_cancel_action(ModContext*,void*,void*,void*){
    if(s_actionScopes.empty())return;
    const auto scope=s_actionScopes.back();s_actionScopes.pop_back();if(!scope.changed)return;
    scope.link->mItemButton=(scope.link->mItemButton&~daAlink_c::BTN_A)|(scope.held&daAlink_c::BTN_A);
    scope.link->mItemTrigger=(scope.link->mItemTrigger&~daAlink_c::BTN_A)|(scope.pressed&daAlink_c::BTN_A);
    g_dComIfG_gameInfo.play.mItemInfo.mDoStatus=scope.status;
}

bool boss_context(){bool value=false;return svc_game_mode&&
    svc_game_mode->is_active(mod_ctx,"bossrush",&value)==MOD_OK&&value;}
const char* blob_name(){return s_boss?"kcrux-five-items.bossrush.v1":"kcrux-five-items.normal.v1";}
int logical_slot(int native){
    if(native<0||native>2)return -1;
    if(native==s_fishingProxy&&s_fishingRodSlot>=2)return s_fishingRodSlot;
    if(native==0&&s_tradeSlot>=2)return s_tradeSlot;
    if(native==0&&s_reelRodSlot>=2)return s_reelRodSlot;
    if(native==2&&s_querySlot>=0)return s_querySlot;
    return native==2&&s_active>=2&&dMeter2Info_getWindowStatus()==0?s_active:native;
}
u8 resolved_item(int logical){
    if(logical<0||logical>=5)return dItemNo_NONE_e;
    const auto a=s_slots.values[logical];
    if(a.item>=MAX_ITEM_SLOTS)return dItemNo_NONE_e;
    u8 first=dComIfGs_getItem(a.item,false);
    if(a.mix>=MAX_ITEM_SLOTS)return first;
    u8 second=dComIfGs_getItem(a.mix,false);
    if(second==dItemNo_BOW_e||second==dItemNo_FISHING_ROD_1_e)std::swap(first,second);
    if(first==dItemNo_BOW_e){
        if(second==dItemNo_NORMAL_BOMB_e||second==dItemNo_WATER_BOMB_e||second==dItemNo_POKE_BOMB_e)return dItemNo_BOMB_ARROW_e;
        if(second==dItemNo_HAWK_EYE_e)return dItemNo_HAWK_ARROW_e;
    }else if(first==dItemNo_FISHING_ROD_1_e){
        if(second==dItemNo_BEE_CHILD_e)return dItemNo_BEE_ROD_e;
        if(second==dItemNo_WORM_e)return dItemNo_WORM_ROD_e;
        if(second==dItemNo_ZORAS_JEWEL_e)return dItemNo_JEWEL_ROD_e;
    }
    return first;
}
void sync_native(){
    // These three saved entries are compatibility mirrors, never an input to the HUD.
    auto& status=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA();
    for(int i=0;i<3;++i){status.mSelectItem[i]=s_slots.values[i].item;status.mMixItem[i]=s_slots.values[i].mix;
        g_dComIfG_gameInfo.play.setSelectItem(i,resolved_item(logical_slot(i)));}
}
bool persist(){
    if(!svc_save)return false;
    const auto data=s_slots.serialize();
    return svc_save->set_blob(mod_ctx,blob_name(),data.data(),data.size())==MOD_OK;
}
void reset_transient(){s_active=-1;s_owner=nullptr;s_actor=0;s_fireState.reset();s_wheelRequest=-1;s_mapWarp={};
    s_fishingRodSlot=-1;s_fishingProxy=-1;s_fishingOwner=nullptr;s_fishingStarted=false;
    s_tradeSlot=-1;s_tradeFrames=0;s_tradeOwner=nullptr;s_tradePending=false;s_tradeSeenEvent=false;
    s_reelRodSlot=-1;s_reelActor=nullptr;}
void load_slots(ModContext*,u32,void*){
    if(!kcrux_controls_enabled()){s_loaded=false;reset_transient();return;}
    reset_transient();s_boss=boss_context();s_slots={};
    std::array<u8,kcrux::Slots::SerializedSize> bytes{};size_t size=bytes.size();
    const bool found=svc_save&&svc_save->get_blob(mod_ctx,blob_name(),bytes.data(),&size)==MOD_OK&&
        s_slots.deserialize(bytes.data(),size,MAX_ITEM_SLOTS);
    if(!found){
        const auto& status=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA();
        for(int i=0;i<3;++i)s_slots.values[i]={status.mSelectItem[i],status.mMixItem[i]};
        u8 extra[2]={255,255};size=2;
        if(svc_save&&svc_save->get_blob(mod_ctx,s_boss?"kcrux-items.bossrush.v1":"kcrux-items.normal.v1",extra,&size)==MOD_OK&&size==2)
            for(int i=0;i<2;++i)s_slots.values[i+3]={extra[i],255};
        const unsigned changed=s_slots.normalize(MAX_ITEM_SLOTS);
        if(changed)svc_log->warn(mod_ctx,"Rewired migration removed duplicate or invalid item assignments; check the five item indicators");
    }
    s_loaded=true;++s_revision;sync_native();
    if(!found)persist();
}
void new_slots(ModContext*,u32,void*){
    if(!kcrux_controls_enabled()){s_loaded=false;reset_transient();return;}
    s_boss=boss_context();s_slots={};s_loaded=true;reset_transient();++s_revision;sync_native();persist();
}
HookAction select_index(ModContext*,void* args,void* result,void*){
    const int logical=logical_slot(mods::arg<int>(args,0));
    if(!kcrux_controls_enabled()||!s_loaded||logical<0)return HOOK_CONTINUE;
    *static_cast<u8*>(result)=s_slots.values[logical].item;return HOOK_SKIP_ORIGINAL;
}
HookAction mix_index(ModContext*,void* args,void* result,void*){
    const int logical=logical_slot(mods::arg<int>(args,0));
    if(!kcrux_controls_enabled()||!s_loaded||logical<0)return HOOK_CONTINUE;
    *static_cast<u8*>(result)=s_slots.values[logical].mix;return HOOK_SKIP_ORIGINAL;
}
HookAction play_item(ModContext*,void* args,void* result,void*){
    const int logical=logical_slot(mods::arg<int>(args,0));
    if(!kcrux_controls_enabled()||!s_loaded||logical<0)return HOOK_CONTINUE;
    *static_cast<u8*>(result)=resolved_item(logical);return HOOK_SKIP_ORIGINAL;
}
HookAction set_play_item(ModContext*,void* args,void*,void*){
    const int index=mods::arg<int>(args,0),logical=logical_slot(index);
    if(!kcrux_controls_enabled()||!s_loaded||logical<0)return HOOK_CONTINUE;
    // Native setSelectItem recursively clears an unavailable saved assignment.
    // Canonical assignments intentionally survive depletion/unavailability, so
    // publish only the resolved playback value and retain the committed mirror.
    g_dComIfG_gameInfo.play.setSelectItem(index,resolved_item(logical));
    auto& status=g_dComIfG_gameInfo.info.getPlayer().getPlayerStatusA();
    status.mSelectItem[index]=s_slots.values[index].item;
    status.mMixItem[index]=s_slots.values[index].mix;
    return HOOK_SKIP_ORIGINAL;
}
void after_item_set_button(ModContext*,void* args,void* retval,void*){
    if(!kcrux_controls_enabled()||!s_loaded||mods::arg<int>(args,1)!=dItemNo_KANTERA_e)return;
    auto* found=static_cast<int*>(retval);
    if(!found||*found!=2)return;
    // Vanilla only checks X/Y. For the lantern/oil interaction its callers
    // need a yes/no answer, so include lantern assignments in Down/Up/RT too.
    for(int i=2;i<5;++i){
        if(resolved_item(i)==dItemNo_KANTERA_e){*found=0;return;}
    }
}
void clear_fishing_projection(bool equipRod){
    const int rod=s_fishingRodSlot;
    auto* link=s_fishingOwner;
    s_fishingRodSlot=-1;s_fishingProxy=-1;s_fishingOwner=nullptr;s_fishingStarted=false;
    if(equipRod&&rod>=2&&link){s_active=rod;link->mSelectItemId=2;}
    if(s_loaded)sync_native();
}
HookAction before_fishing_item_change(ModContext*,void* args,void* retval,void*){
    auto* link=mods::arg<daAlink_c*>(args,0);
    const int baitNative=mods::arg<u8>(args,1);
    if(!kcrux_controls_enabled()||!s_loaded||!link||link->checkWolf()||
       baitNative<0||baitNative>2||s_fishingRodSlot>=0||
       !link->itemTriggerCheck(1<<baitNative))return HOOK_CONTINUE;
    const u8 bait=dComIfGp_getSelectItem(baitNative);
    if(baitNative==2&&link->checkTradeItem(bait)&&
       link->field_0x27f8&&link->checkRequestTalkActor(link->mAttList2,link->field_0x27f8)){
        // Let checkNormalAction/orderTalk handle showing the trade item to
        // the nearby actor instead of starting the generic Not Use animation.
        *static_cast<int*>(retval)=0;
        return HOOK_SKIP_ORIGINAL;
    }
    if(bait!=dItemNo_WORM_e&&bait!=dItemNo_BEE_CHILD_e)return HOOK_CONTINUE;
    const int rod=kcrux_find_equipped_slot(link,0x108);
    if(rod<2)return HOOK_CONTINUE;
    // The native bait action needs two simultaneously addressable items.
    // Keep the bait on its real native input, and lend the other X/Y query
    // position to the rod until the fishing procedure has finished.
    s_fishingRodSlot=rod;s_fishingProxy=baitNative==0?1:0;
    s_fishingOwner=link;s_fishingStarted=false;sync_native();
    return HOOK_CONTINUE;
}
void after_fishing_item_change(ModContext*,void* args,void* retval,void*){
    if(s_fishingOwner!=mods::arg<daAlink_c*>(args,0))return;
    if(!retval||*static_cast<int*>(retval)!=7)clear_fishing_projection(false);
}
void after_fishing_init(ModContext*,void* args,void* retval,void*){
    if(s_fishingOwner!=mods::arg<daAlink_c*>(args,0))return;
    if(!retval||!*static_cast<int*>(retval))clear_fishing_projection(false);
    else s_fishingStarted=true;
}
void after_equipped_item_query(ModContext*,void* args,void* retval,void*){
    if(!kcrux_controls_enabled()||!s_loaded||!retval||*static_cast<u16*>(retval))return;
    auto* node=mods::arg<mesg_flow_node_branch*>(args,1);
    if(!node)return;
    const u8 wanted=node->param;
    for(int i=0;i<5;++i){
        const auto slot=s_slots.values[i];
        if(slot.item<MAX_ITEM_SLOTS&&dComIfGs_getItem(slot.item,true)==wanted){
            *static_cast<u16*>(retval)=1;return;
        }
        if(slot.mix<MAX_ITEM_SLOTS&&dComIfGs_getItem(slot.mix,true)==wanted){
            *static_cast<u16*>(retval)=1;return;
        }
    }
}
void clear_trade_projection(){
    s_tradeSlot=-1;s_tradeFrames=0;s_tradeOwner=nullptr;s_tradePending=false;s_tradeSeenEvent=false;
    if(s_loaded)sync_native();
}
HookAction before_trade_talk(ModContext*,void* args,void*,void*){
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!kcrux_controls_enabled()||!s_loaded||!link||link->checkWolf()||
       s_tradeSlot>=0||s_fishingRodSlot>=0||s_active<2||s_active>=5)return HOOK_CONTINUE;
    const bool pressed=s_active<4?s_dpad[s_active].pressed:s_fire.button.pressed;
    const bool nativePressed=link->itemTriggerCheck(daAlink_c::BTN_Z);
    if((!pressed&&!nativePressed)||link->talkTrigger()||
       !link->checkTradeItem(resolved_item(s_active)))return HOOK_CONTINUE;
    s_tradeSlot=s_active;s_tradeOwner=link;s_tradeFrames=0;s_tradePending=true;
    s_tradeTrigger=link->mItemTrigger;
    link->mItemTrigger|=daAlink_c::BTN_X;
    sync_native();
    return HOOK_CONTINUE;
}
void after_trade_talk(ModContext*,void* args,void* retval,void*){
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!s_tradePending||link!=s_tradeOwner)return;
    link->mItemTrigger=s_tradeTrigger;s_tradePending=false;
    if(!retval||!*static_cast<int*>(retval))clear_trade_projection();
}
HookAction before_rod_execute(ModContext*,void* args,void*,void*){
    auto* rodActor=mods::arg<dmg_rod_class*>(args,0);
    auto* link=daAlink_getAlinkActorClass();
    if(!kcrux_controls_enabled()||!s_loaded||!rodActor||!link||link->checkWolf()||
       s_fishingRodSlot>=0||s_tradeSlot>=0||s_reelRodSlot>=0||
       !link->checkFishingRodItem(link->mEquipItem))return HOOK_CONTINUE;
    const int slot=kcrux_find_equipped_slot(link,0x108);
    if(slot<2)return HOOK_CONTINUE;
    const bool held=slot<4?s_dpad[slot].held:s_fire.button.held;
    if(!held)return HOOK_CONTINUE;
    // Dusklight's optional Button Fishing path tests only X/Y item positions
    // and their held buttons. Project the held extra slot into X for this
    // actor update, then put the native input and slot view back immediately.
    auto& input=mDoCPd_c::getCpadInfo(PAD_1);
    s_reelRodSlot=slot;s_reelActor=rodActor;s_reelHeld=input.mButtonFlags;
    input.mButtonFlags|=PAD_BUTTON_X;
    return HOOK_CONTINUE;
}
void after_rod_execute(ModContext*,void* args,void*,void*){
    if(s_reelActor!=mods::arg<dmg_rod_class*>(args,0))return;
    mDoCPd_c::getCpadInfo(PAD_1).mButtonFlags=s_reelHeld;
    s_reelRodSlot=-1;s_reelActor=nullptr;
}
bool commit(const kcrux::Slots& next){
    const auto previous=s_slots;s_slots=next;
    if(!persist()){s_slots=previous;svc_log->warn(mod_ctx,"Rewired assignment was not changed: save storage unavailable");return false;}
    ++s_revision;sync_native();return true;
}
struct Combination {u8 base=255;int owner=-1;bool removing=false;};
Combination combination_for(u8 inventory){
    if(inventory>=MAX_ITEM_SLOTS)return {};
    const u8 item=dComIfGs_getItem(inventory,false);
    const bool bowPart=item==dItemNo_NORMAL_BOMB_e||item==dItemNo_WATER_BOMB_e||item==dItemNo_POKE_BOMB_e||item==dItemNo_HAWK_EYE_e;
    const bool rodPart=item==dItemNo_BEE_CHILD_e||item==dItemNo_WORM_e||item==dItemNo_ZORAS_JEWEL_e;
    if(!bowPart&&!rodPart)return {};
    const u8 baseType=bowPart?dItemNo_BOW_e:dItemNo_FISHING_ROD_1_e;
    for(int base=0;base<MAX_ITEM_SLOTS;++base)if(dComIfGs_getItem(base,false)==baseType){
        const int owner=s_slots.owner(base);
        if(owner>=0)return {static_cast<u8>(base),owner,s_slots.values[owner].mix!=255&&s_slots.values[owner].contains(inventory)};
    }
    return {};
}
bool combine_highlighted(u8 inventory){
    const auto combo=combination_for(inventory);if(combo.owner<0)return false;
    auto next=s_slots;
    if(combo.removing)next.values[combo.owner]={combo.base,255};
    else next.combine(combo.owner,combo.base,inventory);
    if(!commit(next))return false;
    if(auto* audio=Z2GetAudioMgr())audio->seStart(combo.removing?Z2SE_SY_ITEM_COMBINE_OFF:Z2SE_SY_ITEM_COMBINE_ON,
        nullptr,0,0,1.0f,1.0f,-1.0f,-1.0f,0);
    return true;
}
HookAction ring_mix_available(void* args,void* result,bool removing){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    auto* ring=mods::arg<dMenu_Ring_c*>(args,0);
    const auto combo=ring&&!ring->mPlayerIsWolf&&ring->mCurrentSlot<ring->mItemsTotal
        ?combination_for(ring->mItemSlots[ring->mCurrentSlot]):Combination{};
    *static_cast<bool*>(result)=combo.owner>=0&&combo.removing==removing;
    return HOOK_SKIP_ORIGINAL;
}
HookAction ring_mix_on(ModContext*,void* args,void* result,void*){return ring_mix_available(args,result,false);}
HookAction ring_mix_off(ModContext*,void* args,void* result,void*){return ring_mix_available(args,result,true);}
HookAction assign_from_wheel(ModContext*,void* args,void*,void*){
    if(!kcrux_controls_enabled())return HOOK_CONTINUE;
    auto* ring=mods::arg<dMenu_Ring_c*>(args,0);
    if(!ring||dMeter2Info_getWindowStatus()!=2)return HOOK_SKIP_ORIGINAL;
    const int target=s_wheelRequest;s_wheelRequest=-1;
    if(target<0&&!s_wheelCombine.pressed)return HOOK_SKIP_ORIGINAL;
    if(ring->mCurrentSlot>=ring->mItemsTotal||ring->mStatus!=dMenu_Ring_c::STATUS_WAIT||
        ring->mOldStatus==dMenu_Ring_c::STATUS_EXPLAIN_FORCE||ring->mOldStatus==dMenu_Ring_c::STATUS_EXPLAIN||
        !ring->mpItemExplain||ring->mpItemExplain->getStatus()!=0||ring->mPlayerIsWolf)return HOOK_SKIP_ORIGINAL;
    const u8 inventory=ring->mItemSlots[ring->mCurrentSlot];
    if(inventory>=MAX_ITEM_SLOTS||dComIfGs_getItem(inventory,false)==dItemNo_NONE_e)return HOOK_SKIP_ORIGINAL;
    const bool changed=target>=0?kcrux_assign_item_from_menu(target,inventory):combine_highlighted(inventory);
    if(changed){
        // Disable legacy three-channel assignment animations/markers. The new HUD
        // and wheel badges read the committed five-slot registry directly.
        for(int i=0;i<4;++i){ring->field_0x674[i]=0;ring->field_0x6b4[i]=255;ring->field_0x6b8[i]=255;}
        ring->mXButtonSlot=ring->mYButtonSlot=ring->field_0x6ac=255;
        ring->setStatus(dMenu_Ring_c::STATUS_WAIT);ring->stick_wait_init();
    }
    return HOOK_SKIP_ORIGINAL;
}
bool ui_visible(){bool visible=true;return !svc_ui||svc_ui->is_any_document_visible(mod_ctx,&visible)!=MOD_OK||visible;}
bool blocked(){auto* link=daAlink_getAlinkActorClass();return !link||link->checkEventRun()||dComIfGp_event_runCheck()||
    dComIfGp_isPauseFlag()||dMeter2Info_getWindowStatus()!=0||ui_visible();}
void after_pad_read(ModContext*,void*,void*,void*){
    if(!kcrux_controls_enabled())return;
    auto* link=daAlink_getAlinkActorClass();
    if(!s_loaded||s_boss!=boss_context()){
        if(!link)return;
        load_slots(nullptr,0,nullptr);
    }
    const u32 actor=link?fopAcM_GetID(link):0;
    const bool formChanged=link&&link==s_owner&&actor==s_actor&&bool(link->checkWolf())!=s_wolf;
    if(link!=s_owner||actor!=s_actor||formChanged){reset_transient();s_owner=link;s_actor=actor;s_wolf=link&&link->checkWolf();}
    if(s_fishingStarted&&link==s_fishingOwner&&link->mProcID!=daAlink_c::PROC_FISHING_FOOD)
        clear_fishing_projection(true);
    if(s_tradeSlot>=0&&link==s_tradeOwner&&!s_tradePending){
        if(link->checkEventRun())s_tradeSeenEvent=true;
        if((s_tradeSeenEvent&&!link->checkEventRun())||
           (!s_tradeSeenEvent&&++s_tradeFrames>45))
            clear_trade_projection();
    }
    const bool stop=blocked()||formChanged,ring=dMeter2Info_getWindowStatus()==2&&!ui_visible();
    const auto index=PADGetIndexForPort(PAD_1);auto* pad=index<0?nullptr:PADGetSDLGamepadForIndex(index);
    const auto button=[&](int b){return pad&&s_button&&s_button(pad,b);};
    const int axis=pad&&s_axis?s_axis(pad,5):0;
    auto& input=mDoCPd_c::getCpadInfo(PAD_1);
    const bool fullMap=dMeter2Info_getWindowStatus()==4&&!ui_visible();
    s_mapWarp.sample(button(3),!fullMap);
    if(fullMap){
        // Native Z selects Warp on the full map. Y is contextual here, never
        // an item assignment or jump. Native map eligibility checks still run.
        input.mButtonFlags&=~(PAD_TRIGGER_Z|PAD_BUTTON_Y);
        input.mPressedButtonFlags&=~(PAD_TRIGGER_Z|PAD_BUTTON_Y);
        if(s_mapWarp.held)input.mButtonFlags|=PAD_TRIGGER_Z;
        if(s_mapWarp.pressed)input.mPressedButtonFlags|=PAD_TRIGGER_Z;
    }
    const bool collection=dMeter2Info_getWindowStatus()==3&&!ui_visible();
    if(collection&&!s_collectionOpen){s_collectionPage=-1;s_menuLeft={};s_menuRight={};s_menuDpad={};s_menuTrigger={};}
    s_collectionOpen=collection;
    if(!collection||s_collectionPage!=1){s_collectionTarget=-1;s_collectionCombine=false;s_collectionSelection=255;s_menuCombine={};}
    if(collection){
        s_menuLeft.sample(button(9),false);s_menuRight.sample(button(10),false);
        input.mButtonFlags&=~(PAD_TRIGGER_L|PAD_TRIGGER_R);
        input.mPressedButtonFlags&=~(PAD_TRIGGER_L|PAD_TRIGGER_R);
        if(s_menuLeft.held)input.mButtonFlags|=PAD_TRIGGER_L;
        if(s_menuLeft.pressed)input.mPressedButtonFlags|=PAD_TRIGGER_L;
        if(s_menuRight.held)input.mButtonFlags|=PAD_TRIGGER_R;
        if(s_menuRight.pressed)input.mPressedButtonFlags|=PAD_TRIGGER_R;
        if(s_collectionPage==1){
            s_menuCombine.sample(button(0),false);
            if(combination_for(s_collectionSelection).owner>=0){
                if(s_menuCombine.pressed)s_collectionCombine=true;
                input.mButtonFlags&=~PAD_BUTTON_A;input.mPressedButtonFlags&=~PAD_BUTTON_A;
            }
            constexpr int physicalMenu[4]={13,14,11,12};
            for(int i=0;i<4;++i)s_menuDpad[i].sample(button(physicalMenu[i]),false);
            s_menuTrigger.sample(axis,false);
            for(int i=0;i<4;++i)if(s_menuDpad[i].pressed){s_collectionTarget=i;break;}
            if(s_menuTrigger.button.pressed)s_collectionTarget=4;
            constexpr u32 oldAssignments=PAD_BUTTON_X|PAD_BUTTON_Y|PAD_TRIGGER_Z;
            constexpr u32 directions=PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_BUTTON_UP|PAD_BUTTON_DOWN;
            input.mButtonFlags&=~(oldAssignments|directions);
            input.mPressedButtonFlags&=~(oldAssignments|directions);

        }else{s_menuDpad={};s_menuTrigger={};}
        return;
    }
    s_menuLeft={};s_menuRight={};s_menuDpad={};s_menuTrigger={};
    constexpr int physical[4]={13,14,11,12};
    for(int i=0;i<4;++i)s_dpad[i].sample(button(physical[i]),stop&&!ring);
    s_items.sample(button(10),stop&&!ring);
    s_quick.sample(axis,stop&&!ring);s_fire.sample(axis,stop);
    s_target.sample(pad&&s_axis?s_axis(pad,4):0,stop);
    s_view.sample(button(4),stop&&!ring);s_sword.sample(button(2),stop);s_cancel.sample(button(1),stop);s_object.sample(button(3),stop);
    s_wheelCombine.sample(button(3),!ring);
    if(ring){
        for(int i=0;i<4;++i)if(s_dpad[i].pressed){s_wheelRequest=i;break;}
        if(s_quick.button.pressed)s_wheelRequest=4;
    }else s_wheelRequest=-1;
    // Wii gameplay also treats D-pad directions as Map/Items shortcuts. The
    // physical D-pad belongs to our item slots. Wheel and both map screens
    // also suppress the legacy D-pad open/close shortcuts.
    if((dMeter2Info_getWindowStatus()==0||ring||dMeter2Info_getWindowStatus()==4||dMeter2Info_getWindowStatus()==5)&&!ui_visible()){
        constexpr u32 directions=PAD_BUTTON_LEFT|PAD_BUTTON_RIGHT|PAD_BUTTON_UP|PAD_BUTTON_DOWN;
        input.mButtonFlags&=~directions;
        input.mPressedButtonFlags&=~directions;
    }
    // The Wii item wheel is opened by native Up. Feed that consumer from the
    // physical RB alone, including while the wheel is open and RB is held.
    if(!stop||ring){
        if(s_items.held)input.mButtonFlags|=PAD_BUTTON_UP;
        if(s_items.pressed)input.mPressedButtonFlags|=PAD_BUTTON_UP;
    }
    if(!stop){
        input.mButtonFlags&=~(PAD_BUTTON_B|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_TRIGGER_Z);
        input.mPressedButtonFlags&=~(PAD_BUTTON_B|PAD_BUTTON_X|PAD_BUTTON_Y|PAD_TRIGGER_Z);
        if(s_sword.held)input.mButtonFlags|=PAD_BUTTON_B;
        if(s_sword.pressed)input.mPressedButtonFlags|=PAD_BUTTON_B;
        if(s_wolf){
            // Native wolf X/Y are Senses/Dig, independent of human inventory.
            // Physical Up/Down supply native Senses/Dig. Left, Right and RT
            // cannot select/equip a human item in this form.
            s_active=-1;s_fireState.reset();
            for(int i=0;i<2;++i){
                if(s_dpad[i+2].pressed)input.mPressedButtonFlags|=NativeFlags[i];
                if(s_dpad[i+2].held)input.mButtonFlags|=NativeFlags[i];
            }
        }else{
            int request=-1;for(int i=0;i<4;++i)if(s_dpad[i].pressed){request=i;break;}
            if(request<0&&!kcrux_aiming_item(link)&&s_fire.button.pressed)request=4;
            if(s_sword.pressed){s_active=-1;sync_native();}
            if(request>=0&&s_slots.values[request].item!=255){s_active=request;sync_native();
                input.mPressedButtonFlags|=NativeFlags[request<2?request:2];}
            if(s_active>=0){const bool held=s_active<4?s_dpad[s_active].held:s_fire.button.held;
                if(held)input.mButtonFlags|=NativeFlags[s_active<2?s_active:2];}
        }
    }
    if(!stop||ring){input.mHoldLockR=s_view.held;input.mTrigLockR=s_view.pressed;
        input.mButtonFlags&=~PAD_TRIGGER_R;input.mPressedButtonFlags&=~PAD_TRIGGER_R;
        if(s_view.held)input.mButtonFlags|=PAD_TRIGGER_R;
        if(s_view.pressed)input.mPressedButtonFlags|=PAD_TRIGGER_R;
        input.mTriggerRight=s_view.held?1.f:0.f;}
    if(!stop){
        input.mButtonFlags&=~PAD_TRIGGER_L;input.mPressedButtonFlags&=~PAD_TRIGGER_L;
        if(s_target.button.held)input.mButtonFlags|=PAD_TRIGGER_L;
        if(s_target.button.pressed)input.mPressedButtonFlags|=PAD_TRIGGER_L;
        input.mHoldLockL=s_target.button.held;input.mTrigLockL=s_target.button.pressed;
        input.mTriggerLeft=s_target.button.held?1.f:0.f;
    }
    if(stop)s_fireState.reset();
}
bool fire_ready(daAlink_c* link){
    if(link->checkBowAndSlingItem(link->mEquipItem))return link->checkBowChargeWaitAnime();
    if(link->mEquipItem==dItemNo_BOOMERANG_e)return link->checkBoomerangReadyAnime();
    if(link->checkHookshotItem(link->mEquipItem))return link->checkHookshotWait();
    if(link->checkIronBallEquip())return link->checkIronBallSwingAnime();
    if(link->checkCopyRodEquip())return link->checkCopyRodReadyAnime();return false;
}
void after_stick_data(ModContext*,void* args,void*,void*){
    auto* link=mods::arg<daAlink_c*>(args,0);
    if(!kcrux_controls_enabled()||!link||blocked()){s_fireState.reset();return;}
    const bool aiming=kcrux_aiming_item(link);
    link->mItemButton&=~daAlink_c::BTN_B;link->mItemTrigger&=~daAlink_c::BTN_B;
    if(s_sword.held)link->mItemButton|=daAlink_c::BTN_B;
    if(s_sword.pressed)link->mItemTrigger|=daAlink_c::BTN_B;
    if(aiming&&s_cancel.pressed){link->mItemTrigger|=daAlink_c::BTN_A;s_fireState.reset();return;}
    if(!aiming||link->mSelectItemId>=3){s_fireState.reset();return;}
    const u8 bit=1u<<link->mSelectItemId;
    const bool request=s_fire.button.pressed||(link->mItemTrigger&bit)!=0;
    const bool fire=s_fireState.update(reinterpret_cast<std::uintptr_t>(link),link->mEquipItem,
        s_active<0?link->mSelectItemId:s_active,false,request,fire_ready(link));
    link->mItemTrigger&=~bit;if(fire)link->mItemButton&=~bit;else link->mItemButton|=bit;
}
}
bool kcrux_controls_enabled(){return s_modeAtBoot;}
bool kcrux_jump_button_pressed(){return kcrux_controls_enabled()&&s_object.pressed&&!blocked();}
int kcrux_collection_page(){return kcrux_controls_enabled()&&s_collectionOpen?s_collectionPage:-1;}
void kcrux_assign_collection_selection(unsigned char inventory){
    if(!kcrux_controls_enabled()||dMeter2Info_getWindowStatus()!=3||s_collectionPage!=1)return;
    s_collectionSelection=inventory;
    if(s_collectionTarget>=0){
        const int target=s_collectionTarget;s_collectionTarget=-1;s_collectionCombine=false;
        if(inventory<MAX_ITEM_SLOTS&&dComIfGs_getItem(inventory,false)!=dItemNo_NONE_e)
            kcrux_assign_item_from_menu(target,inventory);
    }else if(s_collectionCombine){s_collectionCombine=false;combine_highlighted(inventory);}
}
int kcrux_combination_state(unsigned char inventory){
    if(!kcrux_controls_enabled()||!s_loaded)return 0;
    const auto combo=combination_for(inventory);
    return combo.owner<0?0:combo.removing?2:1;
}
void kcrux_report_collection_page(int page){if(kcrux_controls_enabled()&&dMeter2Info_getWindowStatus()==3){
    s_collectionPage=page;if(page!=1){s_collectionTarget=-1;s_collectionCombine=false;s_collectionSelection=255;}
}}
unsigned char kcrux_cancel_action(daAlink_c* link){
    if(!kcrux_controls_enabled()||!link)return BUTTON_STATUS_NONE;
    if(kcrux_aiming_item(link))return BUTTON_STATUS_BACK;
    const u8 status=dComIfGp_getDoStatusForce()?dComIfGp_getDoStatusForce():dComIfGp_getDoStatus();
    if(status==BUTTON_STATUS_DISMOUNT)return status;
    return stow_available(link)?BUTTON_STATUS_PUT_AWAY:BUTTON_STATUS_NONE;
}
unsigned char kcrux_push_action(daAlink_c* link){
    if(!kcrux_controls_enabled()||!link||blocked())return BUTTON_STATUS_NONE;
    const u8 status=dComIfGp_getRStatusForce()?dComIfGp_getRStatusForce():dComIfGp_getRStatus();
    switch(status){
    case BUTTON_STATUS_GRAB:case BUTTON_STATUS_PUSH:case BUTTON_STATUS_PULL:
    case BUTTON_STATUS_UNK_150:case BUTTON_STATUS_LOCK:case BUTTON_STATUS_SWITCH:
    case BUTTON_STATUS_SHIELD_ATTACK:case BUTTON_STATUS_DEFEND:case BUTTON_STATUS_COVER:return status;
    case BUTTON_STATUS_PICK_UP:if(link->checkWolf())return status;break;
    default:break;
    }
    // Keep Y reserved while a block/chain is held, even between prompt updates.
    switch(link->mProcID){
    case daAlink_c::PROC_PUSH_PULL_WAIT:case daAlink_c::PROC_PUSH_MOVE:
    case daAlink_c::PROC_PULL_MOVE:return BUTTON_STATUS_GRAB;
    default:return link->checkFmChainGrabAnime()?BUTTON_STATUS_GRAB:BUTTON_STATUS_NONE;
    }
}
bool kcrux_aiming_item(daAlink_c* link) {
    if (!link || link->checkWolf()) return false;
    const auto item = link->mEquipItem;
    const bool supported=link->checkBowAndSlingItem(item) || item == dItemNo_BOOMERANG_e ||
        link->checkHookshotItem(item) || item == dItemNo_IRONBALL_e || item == dItemNo_COPY_ROD_e;
    if (!supported) return false;
    // An item can remain equipped after B cancels its ready view. Do not keep
    // holding its input or queue shots in ordinary movement/carry/cutscene states.
    switch(link->mProcID) {
    case daAlink_c::PROC_BOW_SUBJECT: case daAlink_c::PROC_BOW_MOVE:
    case daAlink_c::PROC_HORSE_BOW_SUBJECT: case daAlink_c::PROC_HORSE_BOW_MOVE:
    case daAlink_c::PROC_CANOE_BOW_SUBJECT: case daAlink_c::PROC_CANOE_BOW_MOVE:
    case daAlink_c::PROC_BOOMERANG_SUBJECT: case daAlink_c::PROC_BOOMERANG_MOVE:
    case daAlink_c::PROC_HORSE_BOOMERANG_SUBJECT: case daAlink_c::PROC_HORSE_BOOMERANG_MOVE:
    case daAlink_c::PROC_CANOE_BOOMERANG_SUBJECT: case daAlink_c::PROC_CANOE_BOOMERANG_MOVE:
    case daAlink_c::PROC_HOOKSHOT_SUBJECT: case daAlink_c::PROC_HOOKSHOT_MOVE:
    case daAlink_c::PROC_HORSE_HOOKSHOT_SUBJECT: case daAlink_c::PROC_HORSE_HOOKSHOT_MOVE:
    case daAlink_c::PROC_CANOE_HOOKSHOT_SUBJECT: case daAlink_c::PROC_CANOE_HOOKSHOT_MOVE:
    case daAlink_c::PROC_SWIM_HOOKSHOT_SUBJECT: case daAlink_c::PROC_SWIM_HOOKSHOT_MOVE:
    case daAlink_c::PROC_HOOKSHOT_ROOF_WAIT: case daAlink_c::PROC_HOOKSHOT_WALL_WAIT:
    case daAlink_c::PROC_COPY_ROD_SUBJECT: case daAlink_c::PROC_COPY_ROD_MOVE:
    case daAlink_c::PROC_IRON_BALL_SUBJECT: case daAlink_c::PROC_IRON_BALL_MOVE:
        return true;
    default: return false;
    }
}


ConfigVarHandle kcrux_controls_config_var(){return s_enabled;}
int kcrux_active_slot(){return s_active;}
int kcrux_find_equipped_slot(const daAlink_c* link,int itemGroup){
    if(!kcrux_controls_enabled()||!s_loaded||!link)return -1;
    for(int i=0;i<5;++i)if(link->checkGroupItem(itemGroup,resolved_item(i)))return i;
    return -1;
}
unsigned char kcrux_resolved_item(int slot){
    return kcrux_controls_enabled()&&s_loaded?resolved_item(slot):dItemNo_NONE_e;
}
int kcrux_project_equipped_item(daAlink_c* link,int itemGroup){
    const int slot=kcrux_find_equipped_slot(link,itemGroup);
    if(slot<0)return -1;
    if(slot>=2){s_active=slot;sync_native();link->mSelectItemId=2;return 2;}
    link->mSelectItemId=slot;return slot;
}
unsigned kcrux_assignment_revision(){return s_revision;}
int kcrux_item_owner(u8 inventory){return s_loaded?s_slots.owner(inventory):-1;}
bool kcrux_assign_item(int slot,u8 inventory){
    if(!s_loaded||slot<0||slot>=5||inventory>=MAX_ITEM_SLOTS)return false;
    auto next=s_slots;next.assign(slot,inventory);return commit(next);
}
bool kcrux_assign_item_from_menu(int slot,u8 inventory){
    if(inventory>=MAX_ITEM_SLOTS||dComIfGs_getItem(inventory,false)==dItemNo_NONE_e)return false;
    const bool clearing=s_slots.owner(inventory)==slot;
    if(!kcrux_assign_item(slot,inventory))return false;
    // Every inventory frontend, including a future native Essentials menu,
    // uses the same transaction and cue. Save loading and gameplay do not.
    if(auto* audio=Z2GetAudioMgr())audio->seStart(clearing?Z2SE_SY_ITEM_COMBINE_OFF:
        Z2SE_SY_ITEM_SET_X,nullptr,0,0,1.0f,1.0f,-1.0f,-1.0f,0);
    return true;
}
KcruxSlotView kcrux_slot_view(int slot){
    KcruxSlotView view;if(!s_loaded||slot<0||slot>=5)return view;
    const auto a=s_slots.values[slot];view.inventory=a.item;view.mix=a.mix;view.item=resolved_item(slot);
    auto* link=daAlink_getAlinkActorClass();
    view.available=view.item!=255&&view.item!=0&&link&&!link->checkWolf();
    if(!view.available)return view;
    const int previous=s_querySlot;s_querySlot=slot;
    view.count=dComIfGp_getSelectItemNum(2);view.maximum=dComIfGp_getSelectItemMaxNum(2);
    s_querySlot=previous;
    if(view.item==dItemNo_BOW_e||view.item==dItemNo_BOMB_ARROW_e||view.item==dItemNo_LIGHT_ARROW_e||view.item==dItemNo_HAWK_ARROW_e||
       view.item==dItemNo_ARROW_LV1_e||view.item==dItemNo_ARROW_LV2_e||view.item==dItemNo_ARROW_LV3_e){
        view.count=dComIfGs_getArrowNum();view.maximum=dComIfGs_getArrowMax();
    }
    if(view.maximum<=0){view.count=-1;view.maximum=0;}
    view.oil=view.item==dItemNo_KANTERA_e||view.item==dItemNo_KANTERA2_e;
    return view;
}
ModResult initialize_kcrux_controls(ModError* error){
    ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;desc.name="modern-controls";desc.type=CONFIG_VAR_BOOL;desc.default_bool=false;
    auto result=svc_config->register_var(mod_ctx,&desc,&s_enabled);
    if(result!=MOD_OK)return mods::set_error(error,result,"could not register custom controls");
    if(svc_config->get_bool(mod_ctx,s_enabled,&s_modeAtBoot)!=MOD_OK)
        return mods::set_error(error,MOD_ERROR,"could not read Rewired mode at launch");
    void* address=nullptr;
    if(svc_hook->resolve(mod_ctx,"SDL_GetGamepadButton",&address,nullptr)==MOD_OK)s_button=reinterpret_cast<ButtonFn>(address);
    address=nullptr;if(svc_hook->resolve(mod_ctx,"SDL_GetGamepadAxis",&address,nullptr)==MOD_OK)s_axis=reinterpret_cast<AxisFn>(address);
    if(!s_button||!s_axis||!svc_save)return mods::set_error(error,MOD_UNAVAILABLE,"custom controls input/save service unavailable");
    HookOptions early=HOOK_OPTIONS_INIT;early.priority=300;
    // TE also edits the completed pad state. Rewired must be the final
    // gameplay projection so its temporary third native channel survives.
    HookOptions late=HOOK_OPTIONS_INIT;late.priority=-1000;
    result=mods::hook::add_post<KcruxPadRead>(svc_hook,after_pad_read,&late);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxMapSample>(svc_hook,sample_map_gesture);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxMapTrig>(svc_hook,map_action_trigger,&early);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredMidnaTrigger>(svc_hook,after_midna_trigger,&late);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxStow>(svc_hook,before_stow,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxStow>(svc_hook,after_cancel_action);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxDismount>(svc_hook,before_dismount,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxDismount>(svc_hook,after_cancel_action);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxGrabTrigger>(svc_hook,before_grab_trigger,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxGrabHold>(svc_hook,before_grab_hold,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxSecondaryTrigger>(svc_hook,before_secondary_trigger,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxSecondaryHold>(svc_hook,before_secondary_hold,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxStickData>(svc_hook,after_stick_data);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxSelectIndex>(svc_hook,select_index,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxMixIndex>(svc_hook,mix_index,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxPlayItem>(svc_hook,play_item,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxSetPlayItem>(svc_hook,set_play_item,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxItemSetButton>(svc_hook,after_item_set_button);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxNewItemChange>(svc_hook,before_fishing_item_change,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxNewItemChange>(svc_hook,after_fishing_item_change);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxFishingFoodInit>(svc_hook,after_fishing_init);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxEquippedItemQuery>(svc_hook,after_equipped_item_query);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxTradeTalk>(svc_hook,before_trade_talk,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxTradeTalk>(svc_hook,after_trade_talk);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxRodExecute>(svc_hook,before_rod_execute,&early);
    if(result==MOD_OK)result=mods::hook::add_post<KcruxRodExecute>(svc_hook,after_rod_execute);
    if(result==MOD_OK)result=mods::hook::add_pre<KcruxRingAssign>(svc_hook,assign_from_wheel,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredMixOn>(svc_hook,ring_mix_on,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredMixOff>(svc_hook,ring_mix_off,&early);
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredCollectionAssign>(svc_hook,assign_from_collection,&early);
    if(result==MOD_OK)result=svc_save->observe_saves(mod_ctx,new_slots,load_slots,nullptr,nullptr,&s_saveObserver);
    if(result!=MOD_OK)return mods::set_error(error,result,"could not install five-slot controls");
    // Save lifecycle owns migration; title-screen data must not be saved as a real slot.
    s_loaded=false;reset_transient();
    svc_log->info(mod_ctx,"Rewired controls revision 27: aiming movement, Y map warp and Zora swim vertical controls");return MOD_OK;
}
void shutdown_kcrux_controls(){
    mods::hook::uninstall<RewiredMixOn>(svc_hook);mods::hook::uninstall<RewiredMixOff>(svc_hook);
    mods::hook::uninstall<RewiredMidnaTrigger>(svc_hook);
    mods::hook::uninstall<KcruxStickData>(svc_hook);mods::hook::uninstall<KcruxPadRead>(svc_hook);
    mods::hook::uninstall<KcruxSelectIndex>(svc_hook);mods::hook::uninstall<KcruxMixIndex>(svc_hook);
    mods::hook::uninstall<KcruxPlayItem>(svc_hook);mods::hook::uninstall<KcruxRingAssign>(svc_hook);
    mods::hook::uninstall<RewiredCollectionAssign>(svc_hook);
    mods::hook::uninstall<KcruxSetPlayItem>(svc_hook);
    mods::hook::uninstall<KcruxItemSetButton>(svc_hook);
    mods::hook::uninstall<KcruxNewItemChange>(svc_hook);
    mods::hook::uninstall<KcruxFishingFoodInit>(svc_hook);
    mods::hook::uninstall<KcruxEquippedItemQuery>(svc_hook);
    mods::hook::uninstall<KcruxTradeTalk>(svc_hook);
    mods::hook::uninstall<KcruxRodExecute>(svc_hook);
    mods::hook::uninstall<KcruxStow>(svc_hook);mods::hook::uninstall<KcruxDismount>(svc_hook);s_actionScopes.clear();
    mods::hook::uninstall<KcruxGrabTrigger>(svc_hook);mods::hook::uninstall<KcruxGrabHold>(svc_hook);
    mods::hook::uninstall<KcruxSecondaryTrigger>(svc_hook);mods::hook::uninstall<KcruxSecondaryHold>(svc_hook);
    mods::hook::uninstall<KcruxMapSample>(svc_hook);mods::hook::uninstall<KcruxMapTrig>(svc_hook);
    s_mapGesture={};s_mapMenu={};s_mapOverride=false;
    if(s_saveObserver&&svc_save)svc_save->unobserve_saves(mod_ctx,s_saveObserver);
    s_saveObserver=0;s_loaded=false;s_slots={};reset_transient();s_dpad={};s_items={};s_sword={};s_cancel={};s_view={};s_object={};s_midna={};s_fire={};s_quick={};s_target={};
    s_collectionOpen=false;s_collectionPage=0;s_collectionTarget=-1;
    s_menuLeft={};s_menuRight={};s_menuDpad={};s_menuTrigger={};
    s_button=nullptr;s_axis=nullptr;s_enabled=0;s_modeAtBoot=false;
}
}
