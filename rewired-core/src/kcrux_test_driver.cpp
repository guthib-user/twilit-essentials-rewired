// Integration fixture, excluded from the production package.
#include "kcrux_test_driver.hpp"
#include "service_imports.hpp"
#include "kcrux_controls.hpp"
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_mg_rod.h"
#include "d/d_com_inf_game.h"
#include "d/d_attention.h"
#include "d/d_msg_flow.h"
#include "d/d_meter2_info.h"
#include "d/d_meter2.h"
#include "d/d_meter2_draw.h"
#include "d/d_meter_map.h"
#include "d/d_map.h"
#include "d/d_pane_class.h"
#include "JSystem/JKernel/JKRArchive.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DPictureEx.h"
#include "JSystem/J2DGraph/J2DOrthoGraph.h"
#include "JSystem/JUtility/JUTFont.h"
#include "JSystem/J2DGraph/J2DScreen.h"
#include "d/d_menu_ring.h"
#include "d/d_menu_collect.h"
#include "f_op/f_op_actor_iter.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_name.h"
#include "mods/svc/hook.hpp"
#include <atomic>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

namespace dawnlight {
namespace {
DEFINE_HOOK_SYMBOL("SDL_GetGamepadButton", bool(SDL_Gamepad*, int), TestButton);
DEFINE_HOOK_SYMBOL("SDL_GetGamepadAxis", short(SDL_Gamepad*, int), TestAxis);
DEFINE_HOOK(&dMenu_Ring_c::_move,TestRingMove);
DEFINE_HOOK(&dMeter2Draw_c::draw,TestContextDraw);
DEFINE_HOOK(static_cast<void (dMeter2Draw_c::*)(f32,f32,f32*,f32,JUtility::TColor,JUtility::TColor,
    JUtility::TColor,JUtility::TColor,f32,u8,f32)>(&dMeter2Draw_c::drawPikari),TestCombinationFlare);
unsigned s_comboSilentFrames=0,s_comboAudibleFrames=0;
HookAction audit_combination_flare(ModContext*,void* args,void*,void*){
    if(dMeter2Info_getWindowStatus()==3&&kcrux_collection_page()==1&&mods::arg<f32>(args,2)>406.f){
        if(mods::arg<u8>(args,10)==2)++s_comboAudibleFrames;
        else if(mods::arg<u8>(args,10)==0)++s_comboSilentFrames;
    }
    return HOOK_CONTINUE;
}
DEFINE_HOOK(&fopAcM_orderTalkItemBtnEvent,TestTradeOrder);
DEFINE_HOOK(&dComIfGs_setSelectItemIndex,TestSelectAssignment);
DEFINE_HOOK(&dMenu_Collect_c::draw,TestCollectDraw);
DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPicture::draw),TestPictureDraw);
DEFINE_HOOK(static_cast<void (J2DPictureEx::*)(f32,f32,f32,f32,bool,bool,bool)>(&J2DPictureEx::draw),TestPictureExDraw);
DEFINE_HOOK(&dSv_player_item_c::getItem,TestInventoryRead);
DEFINE_HOOK(static_cast<void (J2DPicture::*)(f32,f32,Mtx*)>(&J2DPicture::drawSelf),TestPictureSelf);
DEFINE_HOOK(&J2DPictureEx::drawSelf,TestPictureExSelf);
DEFINE_HOOK(&dSv_player_get_item_c::isFirstBit,TestFirstBitRead);
DEFINE_HOOK(&JUTFont::drawString_size_scale,TestMenuText);
std::filesystem::path s_dir;
extern unsigned s_frame;
unsigned s_traceFrame=0;
HookAction trace_picture(ModContext*,void* args,void*,void*){
    if(!s_traceFrame||s_traceFrame+2<s_frame)return HOOK_CONTINUE;
    auto* pic=mods::arg<J2DPicture*>(args,0);
    std::ofstream out(s_dir/"picture-draws.txt",std::ios::app);
    out<<std::hex<<pic->mInfoTag<<std::dec<<" pos=";
    for(int i=1;i<5;++i)out<<mods::arg<f32>(args,i)<<',';
    if(auto* tex=pic->getTexture(0))if(auto* image=tex->getTexInfo())out<<" texture="<<image<<" size="<<image->width<<','<<image->height;
    if(auto* port=dComIfGp_getCurrentGrafPort()){
        auto b=port->getGrafType()==1?*static_cast<J2DOrthoGraph*>(port)->getOrtho():*port->getBounds();
        out<<" ortho="<<b.i.x<<','<<b.i.y<<','<<b.f.x<<','<<b.f.y;
    }
    out<<'\n';return HOOK_CONTINUE;
}
HookAction trace_picture_self(ModContext*,void* args,void*,void*){
    if(!s_traceFrame||s_traceFrame+2<s_frame)return HOOK_CONTINUE;
    auto* pic=mods::arg<J2DPicture*>(args,0);
    std::ofstream out(s_dir/"picture-draws.txt",std::ios::app);
    out<<"SELF "<<std::hex<<pic->mInfoTag<<std::dec<<" alpha="<<unsigned(pic->getAlpha())<<" size="<<pic->getWidth()<<','<<pic->getHeight();
    if(auto* tex=pic->getTexture(0))if(auto* image=tex->getTexInfo())out<<" texture="<<image<<" size="<<image->width<<','<<image->height;
    out<<'\n';return HOOK_CONTINUE;
}
void trace_inventory(ModContext*,void* args,void* result,void*){
    if(!s_traceFrame||s_traceFrame+2<s_frame||dMeter2Info_getWindowStatus()!=3)return;
    std::ofstream out(s_dir/"picture-draws.txt",std::ios::app);
    out<<"INVENTORY "<<mods::arg<int>(args,1)<<" = "<<unsigned(*static_cast<u8*>(result))<<'\n';
}
void trace_firstbit(ModContext*,void* args,void* result,void*){
    if(!s_traceFrame||s_traceFrame+2<s_frame||dMeter2Info_getWindowStatus()!=3)return;
    std::ofstream out(s_dir/"picture-draws.txt",std::ios::app);
    out<<"FIRSTBIT "<<unsigned(mods::arg<u8>(args,1))<<" = "<<*static_cast<int*>(result)<<'\n';
}
HookAction trace_menu_text(ModContext*,void* args,void*,void*){
    if(!s_traceFrame||s_traceFrame+2<s_frame||dMeter2Info_getWindowStatus()!=3)return HOOK_CONTINUE;
    std::ofstream out(s_dir/"picture-draws.txt",std::ios::app);
    out<<"TEXT "<<mods::arg<f32>(args,1)<<','<<mods::arg<f32>(args,2)<<','<<mods::arg<f32>(args,3)<<','<<mods::arg<f32>(args,4)<<' '<<mods::arg<const char*>(args,5)<<'\n';
    return HOOK_CONTINUE;
}
bool s_collectSeen=false,s_collect2d=false,s_collectScreen=false;
int s_collectCursorX=-1,s_collectCursorY=-1;
HookAction trace_collect(ModContext*,void* args,void*,void*){
    auto* menu=mods::arg<dMenu_Collect_c*>(args,0);
    s_collectSeen=bool(menu);s_collect2d=menu&&menu->mpCollect2D;
    s_collectScreen=s_collect2d&&menu->mpCollect2D->mpScreen;
    if(s_collect2d){s_collectCursorX=menu->mpCollect2D->mCursorX;s_collectCursorY=menu->mpCollect2D->mCursorY;}
    return HOOK_CONTINUE;
}
unsigned s_selectCalls=0;
int s_lastSelectSlot=-1,s_lastSelectInventory=-1;
HookAction trace_select_assignment(ModContext*,void* args,void*,void*){
    if(dMeter2Info_getWindowStatus()==3){
        ++s_selectCalls;
        s_lastSelectSlot=mods::arg<int>(args,0);
        s_lastSelectInventory=mods::arg<u8>(args,1);
    }
    return HOOK_CONTINUE;
}
bool s_captureTrade=false;
int s_tradeEventType=-1,s_tradeOrderResult=-1,s_tradeQueriedItem=-1,s_equippedQueryResult=-1;
HookAction capture_trade_order(ModContext*,void* args,void* retval,void*){
    if(!s_captureTrade)return HOOK_CONTINUE;
    s_tradeEventType=mods::arg<u16>(args,0);
    *static_cast<s32*>(retval)=1;
    return HOOK_SKIP_ORIGINAL;
}
int s_visualContext=0;
std::array<u8,6> s_savedContext{};
HookAction context_before(ModContext*,void*,void*,void*){
    auto& info=g_dComIfG_gameInfo.play.mItemInfo;
    s_savedContext={info.mDoStatus,info.mDoSetFlag,info.mRStatus,info.mRSetFlag,info.mYStatus,info.mYSetFlag};
    // Render-only view-model fixture: never expose synthetic actions to physics.
    if(s_visualContext==7){info.mDoStatus=BUTTON_STATUS_UNK_32;info.mDoSetFlag=2;info.mRStatus=BUTTON_STATUS_GRAB;info.mRSetFlag=2;}
    if(s_visualContext==8){info.mYStatus=BUTTON_STATUS_DIG;info.mYSetFlag=2;}
    if(s_visualContext==9){info.mDoStatus=BUTTON_STATUS_SPEAK;info.mDoSetFlag=2;}
    if(s_visualContext==10){info.mDoStatus=BUTTON_STATUS_PUT_AWAY;info.mDoSetFlag=2;}
    return HOOK_CONTINUE;
}
void context_after(ModContext*,void*,void*,void*){
    auto& info=g_dComIfG_gameInfo.play.mItemInfo;
    info.mDoStatus=s_savedContext[0];info.mDoSetFlag=s_savedContext[1];info.mRStatus=s_savedContext[2];info.mRSetFlag=s_savedContext[3];info.mYStatus=s_savedContext[4];info.mYSetFlag=s_savedContext[5];
}
int s_ringSlot=-1,s_ringInventory=-1,s_ringStatus=-1;
void ring_state(ModContext*,void* args,void*,void*){
    auto* ring=mods::arg<dMenu_Ring_c*>(args,0);
    s_ringSlot=ring->mCurrentSlot;s_ringStatus=ring->mStatus;
    s_ringInventory=s_ringSlot<ring->mItemsTotal?ring->mItemSlots[s_ringSlot]:-1;
}
// Public SDL 3.4 virtual-joystick descriptor, kept local to the test fixture.
struct VirtualDesc {
    uint32_t version; uint16_t type,padding,vendor,product,naxes,nbuttons,nballs,nhats,ntouchpads,nsensors,padding2[2];
    uint32_t buttonMask,axisMask; const char* name; const void* touchpads; const void* sensors; void* userdata;
    void* callbacks[8];
};
static_assert(sizeof(VirtualDesc)==136);
using AttachFn=uint32_t(*)(const VirtualDesc*);
using DetachFn=bool(*)(uint32_t);
uint32_t s_virtual=0;DetachFn s_detach=nullptr;bool s_bound=false;
using MappingFn=void(*)(u32,const PADButtonMapping*);
MappingFn s_mappingFn=nullptr;
using PushEventFn=bool(*)(void*);
PushEventFn s_pushEvent=nullptr;
bool s_quitting=false;
unsigned s_frame = 0, s_buttons = 0;
unsigned s_objectPresses = 0;
unsigned s_buttonPolls = 0, s_axisPolls = 0;
int s_rt = 0, s_lx = 0, s_ly = 0, s_lt = 0, s_grant = 0, s_rx=0, s_ry=0;
GfxComputeTypeHandle s_compute = 0;
GfxStageHookHandle s_stage = 0;
WGPUBuffer s_readback = nullptr;
unsigned s_w = 0, s_h = 0, s_row = 0, s_captureNo = 0, s_wait = 0;
std::atomic<bool> s_copied{false};
bool s_request = false, s_mapping = false, s_bgra = false;
bool s_recording=false;
double s_captureTime=0;
int dump_actor(void* ptr,void* data){
    auto* a=static_cast<fopAc_ac_c*>(ptr);auto& out=*static_cast<std::ofstream*>(data);
    out<<fopAcM_GetID(a)<<','<<fopAcM_GetName(a)<<','<<a->current.pos.x<<','<<a->current.pos.y<<','<<a->current.pos.z
       <<','<<a->shape_angle.y<<','<<int(fopAcM_GetRoomNo(a))<<','<<fopAcM_GetParam(a)<<'\n';return 1;
}

HookAction button(ModContext*, void* args, void* result, void*) {
    ++s_buttonPolls;
    const int index = mods::arg<int>(args, 1);
    *static_cast<bool*>(result) = index >= 0 && index < 32 && (s_buttons & (1u << index));
    return HOOK_SKIP_ORIGINAL;
}
HookAction axis(ModContext*, void* args, void* result, void*) {
    ++s_axisPolls;
    const int index = mods::arg<int>(args, 1);
    *static_cast<short*>(result) = index == 5 ? s_rt : index == 4 ? s_lt :
        index == 0 ? s_lx : index == 1 ? s_ly : index==2?s_rx:index==3?s_ry:0;
    return HOOK_SKIP_ORIGINAL;
}
void mapped(WGPUMapAsyncStatus status, WGPUStringView, void*, void*) {
    if (status == WGPUMapAsyncStatus_Success) {
        auto* bytes = static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(s_readback, 0, s_row*s_h));
        std::ofstream out(s_dir / ("capture-" + std::to_string(s_captureNo) + ".ppm"), std::ios::binary);
        out << "P6\n" << s_w << ' ' << s_h << "\n255\n";
        std::vector<char> row(s_w*3);
        for (unsigned y=0; y<s_h; ++y) {
            for (unsigned x=0; x<s_w; ++x) {
                auto* pixel=bytes+y*s_row+x*4;
                row[x*3]=pixel[s_bgra?2:0];row[x*3+1]=pixel[1];row[x*3+2]=pixel[s_bgra?0:2];
            }
            out.write(row.data(),row.size());
        }
        std::ofstream times(s_dir/"capture-times.csv",std::ios::app);
        times.precision(15);times<<s_captureNo<<','<<s_captureTime<<'\n';
        wgpuBufferUnmap(s_readback);
    }
    s_copied.store(false);
}
void copy_frame(ModContext*, const GfxComputeContext* context, const void* payload, size_t, void*) {
    WGPUTexture texture = *static_cast<WGPUTexture const*>(payload);
    WGPUTexelCopyTextureInfo from{}; from.texture=texture; from.aspect=WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo to{}; to.buffer=s_readback; to.layout.bytesPerRow=s_row; to.layout.rowsPerImage=s_h;
    WGPUExtent3D extent{s_w,s_h,1};
    wgpuCommandEncoderCopyTextureToBuffer(context->encoder,&from,&to,&extent);
    s_copied.store(true);
}
void frame_stage(ModContext*, const GfxStageContext*, void*) {
    // Explicit opt-in fixture. Exercises each expanded geometry allocation
    // beyond the stock engine limit, including staging/copy offset agreement.
    static int stressFrames=0;
    if(std::filesystem::exists(s_dir/"renderer-stress.request")) {
        std::filesystem::remove(s_dir/"renderer-stress.request");stressFrames=60;
    }
    if(stressFrames>0) {
        static const std::vector<unsigned char> data(10*1024*1024,0);
        GfxRange vertices{},indices{},storage{};
        const auto v=svc_gfx->push_verts(mod_ctx,data.data(),6*1024*1024,4,&vertices);
        const auto i=svc_gfx->push_indices(mod_ctx,data.data(),3*1024*1024,4,&indices);
        const auto s=svc_gfx->push_storage(mod_ctx,data.data(),10*1024*1024,&storage);
        std::ofstream out(s_dir/"renderer-stress.json");
        out<<"{\"remaining\":"<<--stressFrames<<",\"result\":["<<v<<','<<i<<','<<s<<"]}";
    }
    if (!s_request || s_mapping) return;
    GfxResolveDesc desc=GFX_RESOLVE_DESC_INIT;
    GfxResolvedTargets targets=GFX_RESOLVED_TARGETS_INIT;
    if (svc_gfx->resolve_pass(mod_ctx,&desc,&targets)!=MOD_OK || !targets.colorTexture) return;
    if (s_readback) wgpuBufferRelease(s_readback);
    s_w=targets.width; s_h=targets.height; s_row=(s_w*4+255)&~255u;
    s_bgra=targets.color_format==WGPUTextureFormat_BGRA8Unorm || targets.color_format==WGPUTextureFormat_BGRA8UnormSrgb;
    s_captureTime=std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    GfxDeviceInfo device=GFX_DEVICE_INFO_INIT; svc_gfx->get_device_info(mod_ctx,&device);
    WGPUBufferDescriptor buffer{}; buffer.size=s_row*s_h; buffer.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;
    s_readback=wgpuDeviceCreateBuffer(device.device,&buffer);
    svc_gfx->push_compute(mod_ctx,s_compute,&targets.colorTexture,sizeof(targets.colorTexture));
    s_request=false; s_mapping=true; s_wait=0; ++s_captureNo;
}
}
ModResult initialize_kcrux_test_driver(ModError* error) {
    const char* path=std::getenv("KCRUX_TEST_DIR");
    if (!path || std::string(path).find("control-redesign") == std::string::npos)
        return mods::set_error(error,MOD_ERROR,"test driver requires an explicit isolated project directory");
    s_dir=path;
    void* address=nullptr;
    if(svc_hook->resolve(mod_ctx,"SDL_AttachVirtualJoystick",&address,nullptr)==MOD_OK) {
        VirtualDesc desc{};desc.version=sizeof(desc);desc.type=1;desc.vendor=0x1209;desc.product=0x2026;
        desc.naxes=6;desc.nbuttons=26;desc.buttonMask=0x3ffffff;desc.axisMask=63;desc.name="Rewired Integration Pad";
        s_virtual=reinterpret_cast<AttachFn>(address)(&desc);
    }
    address=nullptr;
    if(svc_hook->resolve(mod_ctx,"SDL_DetachVirtualJoystick",&address,nullptr)==MOD_OK)s_detach=reinterpret_cast<DetachFn>(address);
    address=nullptr;
    if(svc_hook->resolve(mod_ctx,"PADSetAllButtonMappings",&address,nullptr)==MOD_OK)s_mappingFn=reinterpret_cast<MappingFn>(address);
    address=nullptr;
    if(svc_hook->resolve(mod_ctx,"SDL_PushEvent",&address,nullptr)==MOD_OK)s_pushEvent=reinterpret_cast<PushEventFn>(address);
    if(!s_virtual)return mods::set_error(error,MOD_ERROR,"could not attach isolated SDL test gamepad");
    if(!s_mappingFn)return mods::set_error(error,MOD_ERROR,"test gamepad mapping function unavailable");
    auto result=mods::hook::add_pre<TestButton>(svc_hook,button);
    if(result==MOD_OK) result=mods::hook::add_pre<TestAxis>(svc_hook,axis);
    if(result==MOD_OK) result=mods::hook::add_post<TestRingMove>(svc_hook,ring_state);
    if(result==MOD_OK) result=mods::hook::add_pre<TestTradeOrder>(svc_hook,capture_trade_order);
    if(result==MOD_OK) result=mods::hook::add_pre<TestSelectAssignment>(svc_hook,trace_select_assignment);
    if(result==MOD_OK) result=mods::hook::add_pre<TestCollectDraw>(svc_hook,trace_collect);
    if(result==MOD_OK) result=mods::hook::add_pre<TestPictureSelf>(svc_hook,trace_picture_self);
    if(result==MOD_OK) result=mods::hook::add_pre<TestPictureExSelf>(svc_hook,trace_picture_self);
    if(result==MOD_OK) result=mods::hook::add_pre<TestPictureDraw>(svc_hook,trace_picture);
    if(result==MOD_OK) result=mods::hook::add_pre<TestPictureExDraw>(svc_hook,trace_picture);
    if(result==MOD_OK) result=mods::hook::add_post<TestInventoryRead>(svc_hook,trace_inventory);
    if(result==MOD_OK) result=mods::hook::add_post<TestFirstBitRead>(svc_hook,trace_firstbit);
    if(result==MOD_OK) result=mods::hook::add_pre<TestMenuText>(svc_hook,trace_menu_text);
    HookOptions first=HOOK_OPTIONS_INIT;first.priority=1000;
    HookOptions last=HOOK_OPTIONS_INIT;last.priority=-2000;
    if(result==MOD_OK) result=mods::hook::add_pre<TestCombinationFlare>(svc_hook,audit_combination_flare,&last);
    if(result==MOD_OK) result=mods::hook::add_pre<TestContextDraw>(svc_hook,context_before,&first);
    if(result==MOD_OK) result=mods::hook::add_post<TestContextDraw>(svc_hook,context_after,&last);
    GfxComputeTypeDesc compute=GFX_COMPUTE_TYPE_DESC_INIT;compute.callback=copy_frame;compute.label="Integration capture";
    if(result==MOD_OK) result=svc_gfx->register_compute_type(mod_ctx,&compute,&s_compute);
    GfxStageHookDesc stage=GFX_STAGE_HOOK_DESC_INIT;stage.callback=frame_stage;
    if(result==MOD_OK) result=svc_gfx->register_stage_hook(mod_ctx,GFX_STAGE_FRAME_AFTER_HUD,&stage,&s_stage);
    svc_log->info(mod_ctx,"Rewired isolated integration driver active (production build excludes it)");
    return result;
}
void update_kcrux_test_driver() {
    ++s_frame;
    if(std::filesystem::exists(s_dir/"trace-pictures.request")){
        std::filesystem::remove(s_dir/"trace-pictures.request");s_traceFrame=s_frame;
        std::ofstream(s_dir/"picture-draws.txt");
    }
    // Hot-plug order is not stable: select our own virtual device rather than
    // whichever physical controller happens to occupy index zero.
    for(u32 i=0;i<PADCount();++i){
        const auto* name=PADGetNameForControllerIndex(i);
        if(!name||std::strcmp(name,"Rewired Integration Pad")!=0)continue;
        if(s_bound&&PADGetIndexForPort(0)==static_cast<s32>(i))break;
        PADSetPortForIndex(i,0);
        PADButtonMapping map[PAD_BUTTON_COUNT]={{0,PAD_BUTTON_A},{1,PAD_BUTTON_B},{2,PAD_BUTTON_X},{3,PAD_BUTTON_Y},{6,PAD_BUTTON_START},{0xffffffff,PAD_TRIGGER_Z},{9,PAD_TRIGGER_L},{10,PAD_TRIGGER_R},{11,PAD_BUTTON_UP},{12,PAD_BUTTON_DOWN},{13,PAD_BUTTON_LEFT},{14,PAD_BUTTON_RIGHT}};
        s_mappingFn(0,map);s_bound=true;break;
    }
    unsigned buttons=0; int rt=0,lx=0,ly=0,lt=0,grant=0,rx=0,ry=0;
    std::ifstream input(s_dir/"input.txt");input>>buttons>>rt>>lx>>ly>>lt>>grant;input>>rx>>ry;
    if(grant<0 && !s_quitting && s_pushEvent) {
        alignas(8) unsigned char event[128]{};
        *reinterpret_cast<uint32_t*>(event)=0x100; // SDL_EVENT_QUIT: clean engine shutdown.
        s_quitting=true;s_pushEvent(event);return;
    }
    s_buttons=buttons; s_rt=rt; s_lx=lx; s_ly=ly; s_lt=lt;s_rx=rx;s_ry=ry;
    auto* link=daAlink_getAlinkActorClass();
    const bool objectHeld=link&&link->wallGrabButton();
    const bool objectPressed=link&&link->wallGrabTrigger();
    if(objectPressed)++s_objectPresses;
    if(grant && grant!=s_grant && link && !link->checkEventRun()) {
        s_grant=grant;
        if(grant>=7&&grant<=11){s_visualContext=grant==11?0:grant;return;}
        if(grant==12){std::ofstream out(s_dir/"actors.csv");fopAcIt_Executor(dump_actor,&out);return;}
        if(grant==16){
            std::filesystem::create_directories(s_dir/"native-hud");auto* archive=dComIfGp_getMain2DArchive();
            std::ofstream out(s_dir/"native-hud/index.txt");
            for(u32 i=0;i<archive->countFile();++i){auto* entry=archive->findIdxResource(i);if(!entry||entry->isDirectory())continue;
                std::string name=archive->mStringTable+entry->getNameOffset();
                if(name.find(".bti")==std::string::npos)continue;out<<name<<'\n';
                if(name.find("button")==std::string::npos&&name.find("do_icon")==std::string::npos)continue;
                auto* data=archive->getIdxResource(i);if(data){std::ofstream image(s_dir/"native-hud"/name,std::ios::binary);image.write(static_cast<char*>(data),archive->getResSize(data));}}
            out<<"mapEligible="<<dMeterMap_c::isEnableDispMap()<<" sizeType="<<dMeterMap_c::getMapDispSizeTypeNo()
               <<" mapShow="<<dComIfGp_checkMapShow()<<" mapOpen="<<dMeterMap_c::isMapOpenCheck()<<'\n';return;
        }
        if(grant==13){
            cXyz pos;int angle=0;std::ifstream target(s_dir/"position.txt");target>>pos.x>>pos.y>>pos.z>>angle;
            if(target){link->setPlayerPosAndAngle(&pos,static_cast<s16>(angle),true);
                if(link->checkWolf())link->procWolfWaitInit();else link->procWaitInit();}
            return;
        }
        if(grant==14){
            ActorSpawnParams params{};params.parameters=0xF0FFFFFF;params.room_num=fopAcM_GetRoomNo(link);
            params.position={link->current.pos.x,link->current.pos.y+10,link->current.pos.z+250};
            params.angle={0,0,-1};params.scale={1,1,1};ActorId id=0;
            auto* previous=fpcLy_CurrentLayer();fpcLy_SetCurrentLayer(link->layer_tag.layer);
            const auto result=svc_actor->create_actor(mod_ctx,fpcNm_Obj_Movebox_e,&params,&id);fpcLy_SetCurrentLayer(previous);
            std::ofstream(s_dir/"spawn.txt")<<result<<' '<<id;return;
        }
        if(grant==15){
            ActorSpawnParams params{};params.parameters=0x000100FF;params.room_num=fopAcM_GetRoomNo(link);
            params.position={link->current.pos.x,link->current.pos.y,link->current.pos.z};params.scale={1,1,1};ActorId id=0;
            auto* previous=fpcLy_CurrentLayer();fpcLy_SetCurrentLayer(link->layer_tag.layer);
            const auto result=svc_actor->create_actor(mod_ctx,fpcNm_Obj_Digpl_e,&params,&id);fpcLy_SetCurrentLayer(previous);
            std::ofstream(s_dir/"dig-spawn.txt")<<result<<' '<<id;return;
        }
        if(grant==17){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(3,dItemNo_KANTERA_e);
            items.setItem(11,dItemNo_OIL_BOTTLE_e);
            dComIfGs_onItemFirstBit(dItemNo_KANTERA_e);
            dComIfGs_onItemFirstBit(dItemNo_OIL_BOTTLE_e);
            if(kcrux_item_owner(3)!=3)kcrux_assign_item(3,3);
            if(kcrux_item_owner(11)!=4)kcrux_assign_item(4,11);
            dComIfGs_setMaxOil(7200);
            dComIfGs_setOil(0);
            return;
        }
        if(grant==18){
            // Copied-profile fixture for rod/bait on two virtual slots.
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(5,dItemNo_FISHING_ROD_1_e);
            items.setItem(11,dItemNo_WORM_e);
            dComIfGs_onItemFirstBit(dItemNo_FISHING_ROD_1_e);
            dComIfGs_onItemFirstBit(dItemNo_WORM_e);
            dComIfGs_setBottleNum(0,3);
            if(kcrux_item_owner(5)!=3)kcrux_assign_item(3,5);
            if(kcrux_item_owner(11)!=4)kcrux_assign_item(4,11);
            return;
        }
        if(grant==19){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(5,dItemNo_FISHING_ROD_1_e);
            items.setItem(11,dItemNo_WORM_e);
            dComIfGs_onItemFirstBit(dItemNo_FISHING_ROD_1_e);
            dComIfGs_onItemFirstBit(dItemNo_WORM_e);
            dComIfGs_setBottleNum(0,3);
            if(kcrux_item_owner(5)!=4)kcrux_assign_item(4,5);
            if(kcrux_item_owner(11)!=0)kcrux_assign_item(0,11);
            return;
        }
        if(grant==20){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(3,dItemNo_KANTERA_e);
            dComIfGs_onItemFirstBit(dItemNo_KANTERA_e);
            if(kcrux_item_owner(3)!=3)kcrux_assign_item(3,3);
            link->onNoResetFlg2(daPy_py_c::FLG2_UNK_1);
            link->allUnequip(TRUE);
            return;
        }
        if(grant==21){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(5,dItemNo_HVY_BOOTS_e);
            items.setItem(6,dItemNo_SPINNER_e);
            dComIfGs_onItemFirstBit(dItemNo_HVY_BOOTS_e);
            dComIfGs_onItemFirstBit(dItemNo_SPINNER_e);
            if(kcrux_item_owner(5)!=3)kcrux_assign_item(3,5);
            if(kcrux_item_owner(6)!=4)kcrux_assign_item(4,6);
            return;
        }
        if(grant==22){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(7,dItemNo_LETTER_e);
            dComIfGs_onItemFirstBit(dItemNo_LETTER_e);
            if(kcrux_item_owner(7)!=4)kcrux_assign_item(4,7);
            return;
        }
        if(grant==23){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(7,dItemNo_LETTER_e);
            dComIfGs_onItemFirstBit(dItemNo_LETTER_e);
            if(kcrux_item_owner(7)!=4)kcrux_assign_item(4,7);
            kcrux_project_equipped_item(link,dItemNo_LETTER_e);
            dAttList_c attention{};attention.mType=fopAc_attn_SPEAK_e;
            auto* oldList=link->mAttList2;auto* oldTarget=link->field_0x27f8;
            const u8 oldTrigger=link->mItemTrigger;
            link->mAttList2=&attention;link->field_0x27f8=link;
            link->mItemTrigger=daAlink_c::BTN_Z;s_captureTrade=true;
            s_tradeEventType=-1;
            s_tradeOrderResult=link->orderTalk(TRUE);
            s_tradeQueriedItem=dComIfGp_getSelectItem(0);
            s_captureTrade=false;
            link->mItemTrigger=oldTrigger;link->mAttList2=oldList;link->field_0x27f8=oldTarget;
            return;
        }
        if(grant==24){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(7,dItemNo_LETTER_e);
            dComIfGs_onItemFirstBit(dItemNo_LETTER_e);
            if(kcrux_item_owner(7)!=4)kcrux_assign_item(4,7);
            mesg_flow_node_branch node{};node.param=dItemNo_LETTER_e;
            dMsgFlow_c flow{};
            s_equippedQueryResult=flow.query021(&node,nullptr,0);
            return;
        }
        if(grant==25){
            auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
            items.setItem(5,dItemNo_FISHING_ROD_1_e);
            dComIfGs_onItemFirstBit(dItemNo_FISHING_ROD_1_e);
            if(kcrux_item_owner(5)!=3)kcrux_assign_item(3,5);
            kcrux_project_equipped_item(link,0x108);
            link->itemEquip(dItemNo_FISHING_ROD_1_e);
            return;
        }
        if(grant==2) {dComIfGp_setNextStage("F_SP103",0,0,0);return;}
        if(grant==4||grant==5){
            dComIfGs_onEventBit(0x0D04); // Isolated fixture: allow the saved form to select the model.
            dComIfGs_setTransformStatus(grant==4?1:0);
            if(grant==4)dComIfGs_onEventBit(dSv_event_flag_c::F_0550);
            dComIfGp_setNextStage("F_SP103",0,0,0);return;
        }
        if(grant==6){for(int i=0;i<5;++i){const auto item=kcrux_slot_view(i).inventory;
            if(item!=255)kcrux_assign_item(i,item);}return;}
        auto& items=g_dComIfG_gameInfo.info.getPlayer().getItem();
        const unsigned char values[5]={dItemNo_PACHINKO_e,dItemNo_BOW_e,dItemNo_BOOMERANG_e,dItemNo_KANTERA_e,dItemNo_EMPTY_BOTTLE_e};
        for(int i=0;i<5;++i) {items.setItem(i,values[i]);dComIfGs_onItemFirstBit(values[i]);
            if(kcrux_slot_view(i).inventory!=i)kcrux_assign_item(i,i);}
        dComIfGs_setArrowNum(30);dComIfGs_setPachinkoNum(30);
        dComIfGs_setSelectEquipSword(dItemNo_SWORD_e);
    }
    s_recording=std::filesystem::exists(s_dir/"record.flag");
    if((s_recording||std::filesystem::exists(s_dir/"capture.request")) && !s_mapping) {
        std::filesystem::remove(s_dir/"capture.request"); s_request=true;
    }
    const unsigned readbackWait=s_recording?1:5;
    if(s_mapping && s_copied.load() && ++s_wait==readbackWait) {
        WGPUBufferMapCallbackInfo callback{};callback.mode=WGPUCallbackMode_AllowSpontaneous;callback.callback=mapped;
        wgpuBufferMapAsync(s_readback,WGPUMapMode_Read,0,s_row*s_h,callback);
    }
    if(s_mapping && s_wait>=readbackWait && !s_copied.load()) s_mapping=false;
    if(s_frame%3==0) {
        std::ofstream out(s_dir/"state.json");
        out<<"{\"frame\":"<<s_frame<<",\"stage\":\""<<dComIfGp_getStartStageName()<<"\",\"player\":"<<(link?"true":"false")
           <<",\"window\":"<<unsigned(dMeter2Info_getWindowStatus())<<",\"buttons\":"<<s_buttons
           <<",\"buttonPolls\":"<<s_buttonPolls<<",\"axisPolls\":"<<s_axisPolls<<",\"controllers\":"<<PADCount()
           <<",\"nativeHold\":"<<mDoCPd_c::getHold(PAD_1)<<",\"nativeTrig\":"<<mDoCPd_c::getTrig(PAD_1)
           <<",\"nativeLockL\":"<<mDoCPd_c::getHoldLockL(PAD_1)
           <<",\"mapStatus\":"<<unsigned(dMeter2Info_getMapStatus())<<",\"mapShow\":"<<dComIfGp_checkMapShow()
           <<",\"mapEligible\":"<<(link&&dMeterMap_c::isEnableDispMap());
        u32 mappingCount=0;auto* mappings=PADGetButtonMappings(PAD_1,&mappingCount);
        out<<",\"portIndex\":"<<PADGetIndexForPort(PAD_1)<<",\"portName\":\""
           <<(PADGetIndexForPort(PAD_1)>=0?PADGetNameForControllerIndex(PADGetIndexForPort(PAD_1)):"")<<"\",\"mapping\":[";
        for(u32 i=0;i<mappingCount;++i){if(i)out<<',';out<<mappings[i].nativeButton;}out<<']';
        out<<",\"collectSeen\":"<<(s_collectSeen?"true":"false")
           <<",\"collect2d\":"<<(s_collect2d?"true":"false")
           <<",\"collectScreen\":"<<(s_collectScreen?"true":"false")
           <<",\"collectCursor\":["<<s_collectCursorX<<','<<s_collectCursorY<<']'
           <<",\"selectCalls\":"<<s_selectCalls<<",\"lastSelectSlot\":"<<s_lastSelectSlot
           <<",\"lastSelectInventory\":"<<s_lastSelectInventory
           <<",\"tradeEventType\":"<<s_tradeEventType<<",\"tradeOrderResult\":"<<s_tradeOrderResult
           <<",\"tradeQueriedItem\":"<<s_tradeQueriedItem
           <<",\"equippedQueryResult\":"<<s_equippedQueryResult;
        auto* rodActor=reinterpret_cast<dmg_rod_class*>(fopAcM_SearchByName(fpcNm_MG_ROD_e));
        out<<",\"rodActor\":"<<(rodActor?"true":"false")
           <<",\"rodY\":"<<(rodActor?rodActor->getRodStickY():0.f)
           <<",\"rodAction\":"<<(rodActor?rodActor->action:-1);
        if(link) out<<",\"wolf\":"<<(link->checkWolf()?"true":"false")<<",\"senses\":"<<(link->checkWolfEyeUp()?"true":"false")
                    <<",\"proc\":"<<unsigned(link->mProcID)<<",\"item\":"<<unsigned(link->mEquipItem)<<",\"slot\":"<<unsigned(link->mSelectItemId)
                    <<",\"event\":"<<(link->checkEventRun()?"true":"false")<<",\"y\":"<<link->current.pos.y
                    <<",\"x\":"<<link->current.pos.x<<",\"z\":"<<link->current.pos.z
                    <<",\"doStatus\":"<<unsigned(dComIfGp_getDoStatus())<<",\"digEmphasis\":"<<(dComIfGp_isYSetFlag(2)?"true":"false")
                    <<",\"facing\":"<<link->shape_angle.y<<",\"moveAngle\":"<<link->mMoveAngle
                    <<",\"normalSpeed\":"<<link->mNormalSpeed<<",\"maxSpeed\":"<<link->mMaxSpeed
                    <<",\"stickValue\":"<<link->mStickValue<<",\"moveValue\":"<<link->mMoveValue
                    <<",\"moveDirection\":"<<unsigned(link->field_0x2f98)
                    <<",\"itemHeld\":"<<unsigned(link->mItemButton)<<",\"itemTrigger\":"<<unsigned(link->mItemTrigger)
                    <<",\"objectHeld\":"<<(objectHeld?"true":"false")<<",\"objectPresses\":"<<s_objectPresses
                    <<",\"midnaTrigger\":"<<(link->midnaTalkTrigger()?"true":"false")
                    <<",\"objectPrompt\":"<<unsigned(kcrux_push_action(link))
                    <<",\"lanternSet\":"<<link->checkItemSetButton(dItemNo_KANTERA_e)
                    <<",\"rodSet\":"<<link->checkItemSetButton(0x108)
                    <<",\"bootsSet\":"<<link->checkItemSetButton(dItemNo_HVY_BOOTS_e)
                    <<",\"spinnerSet\":"<<link->checkItemSetButton(dItemNo_SPINNER_e)
                    <<",\"bootsEquipped\":"<<(link->checkEquipHeavyBoots()?"true":"false")
                    <<",\"tradeTalkReady\":"<<(link->checkRequestTalkActor(link->mAttList2,link->field_0x27f8)?"true":"false")
                    <<",\"tradeTarget\":"<<(link->field_0x27f8?"true":"false")
                    <<",\"oil\":"<<dComIfGs_getOil()
                    <<",\"maxOil\":"<<dComIfGs_getMaxOil()
                    <<",\"inventoryRod\":"<<unsigned(dComIfGs_getItem(5,false))
                    <<",\"inventoryBait\":"<<unsigned(dComIfGs_getItem(11,false))
                    <<",\"baitCount\":"<<unsigned(dComIfGs_getBottleNum(0))
                    <<",\"arrows\":"<<unsigned(dComIfGs_getArrowNum())<<",\"seeds\":"<<unsigned(dComIfGs_getPachinkoNum());
        out<<",\"ringSlot\":"<<s_ringSlot<<",\"ringInventory\":"<<s_ringInventory<<",\"ringStatus\":"<<s_ringStatus;
        if(link)if(auto* meter=dMeter2Info_getMeterClass())if(auto* map=meter->mpMap)
            out<<",\"mapInside\":"<<unsigned(map->mMapIsInside)<<",\"mapAlpha\":"<<unsigned(map->mMapAlpha)
               <<",\"mapDraw\":"<<(map->mMap&&map->mMap->isDraw())<<",\"mapOffset\":"<<map->mSlidePositionOffset
               <<",\"mapSize\":["<<map->mSizeW<<','<<map->mSizeH<<']';
        bool uiVisible=false;svc_ui->is_any_document_visible(mod_ctx,&uiVisible);
        out<<",\"collectionPage\":"<<kcrux_collection_page()<<",\"uiVisible\":"<<uiVisible;
        out<<",\"inventory\":[";
        for(int i=0;i<24;++i){if(i)out<<',';out<<unsigned(dComIfGs_getItem(i,false));}
        out<<"],\"assignments\":[";
        for(int i=0;i<5;++i){if(i)out<<',';out<<unsigned(kcrux_slot_view(i).inventory);}
        out<<"],\"mixtures\":[";
        for(int i=0;i<5;++i){if(i)out<<',';out<<unsigned(kcrux_slot_view(i).mix);}
        out<<"],\"resolved\":[";
        for(int i=0;i<5;++i){if(i)out<<',';out<<unsigned(kcrux_slot_view(i).item);}
        out<<"],\"counts\":[";
        for(int i=0;i<5;++i){if(i)out<<',';out<<kcrux_slot_view(i).count;}
        out<<"],\"bombs\":"<<unsigned(dComIfGs_getBombNum(0));
        out<<",\"comboSilentFrames\":"<<s_comboSilentFrames<<",\"comboAudibleFrames\":"<<s_comboAudibleFrames;
        out<<",\"activeSlot\":"<<kcrux_active_slot()<<",\"revision\":"<<kcrux_assignment_revision()<<"}";
    }
}
void shutdown_kcrux_test_driver() {
    mods::hook::uninstall<TestCombinationFlare>(svc_hook);
    mods::hook::uninstall<TestPictureSelf>(svc_hook);mods::hook::uninstall<TestPictureExSelf>(svc_hook);mods::hook::uninstall<TestPictureDraw>(svc_hook);mods::hook::uninstall<TestPictureExDraw>(svc_hook);
    mods::hook::uninstall<TestInventoryRead>(svc_hook);mods::hook::uninstall<TestFirstBitRead>(svc_hook);
    mods::hook::uninstall<TestMenuText>(svc_hook);
    mods::hook::uninstall<TestButton>(svc_hook);mods::hook::uninstall<TestAxis>(svc_hook);
    mods::hook::uninstall<TestRingMove>(svc_hook);
    mods::hook::uninstall<TestSelectAssignment>(svc_hook);
    mods::hook::uninstall<TestCollectDraw>(svc_hook);
    mods::hook::uninstall<TestTradeOrder>(svc_hook);
    mods::hook::uninstall<TestContextDraw>(svc_hook);
    if(s_stage)svc_gfx->unregister_stage_hook(mod_ctx,s_stage);
    if(s_compute)svc_gfx->unregister_compute_type(mod_ctx,s_compute);
    if(s_readback)wgpuBufferRelease(s_readback);
    if(s_virtual&&s_detach)s_detach(s_virtual);
    s_virtual=0;s_bound=false;
    s_stage=0;s_compute=0;s_readback=nullptr;
}
}
