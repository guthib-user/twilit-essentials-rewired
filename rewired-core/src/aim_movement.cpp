#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_camera.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/svc/hook.hpp"
#include <algorithm>
#include <vector>

namespace dawnlight {
namespace {
DEFINE_HOOK(&daAlink_c::setBodyAngleToCamera, RewiredAimStick);
ConfigVarHandle s_enabled=0;
struct StickScope {
    daAlink_c* link=nullptr;
    float stick=0,move=0;
    s16 stickAngle=0,moveAngle=0;
};
thread_local std::vector<StickScope> s_scopes;

bool supported(daAlink_c* link) {
    if(!link || link->checkHorseRide() || link->checkEventRun() ||
       link->mEquipItem==dItemNo_HAWK_ARROW_e)return false;
    switch(link->mProcID) {
    case daAlink_c::PROC_BOW_SUBJECT:
    case daAlink_c::PROC_BOOMERANG_SUBJECT:
    case daAlink_c::PROC_HOOKSHOT_SUBJECT:
    case daAlink_c::PROC_IRON_BALL_SUBJECT:
    case daAlink_c::PROC_COPY_ROD_SUBJECT:return true;
    default:return false;
    }
}

HookAction before_aim(ModContext*,void* args,void*,void*) {
    s_scopes.emplace_back();
    auto* link=mods::arg<daAlink_c*>(args,0);
    bool enabled=false;
    if(!kcrux_controls_enabled() || !supported(link) ||
       svc_config->get_bool(mod_ctx,s_enabled,&enabled)!=MOD_OK || !enabled)
        return HOOK_CONTINUE;

    // Keep native item procedures, sights and transitions. Restore the earlier
    // Dawnlight movement/animation behavior at their shared aiming consumer.
    const s16 facing=link->shape_angle.y;
    link->setSpeedAndAngleAtn();
    link->shape_angle.y=facing;
    const bool heavy=link->mProcID==daAlink_c::PROC_IRON_BALL_SUBJECT;
    const u8 direction=heavy||link->mProcID==daAlink_c::PROC_HOOKSHOT_SUBJECT?2:3;
    if(link->checkZeroSpeedF()) {
        link->onModeFlg(1);link->field_0x2f98=direction;
        if(link->mProcID==daAlink_c::PROC_BOW_SUBJECT)
            link->current.angle.y=link->shape_angle.y-0x4000;
    } else link->offModeFlg(1);
    if(heavy && link->checkIronBallPreSwingAnime())link->mNormalSpeed=0;
    if(heavy && link->checkModeFlg(1))link->setIronBallBaseAnime();
    else link->setBlendAtnMoveAnime(-1.f);

    // Only the aiming calculation sees C-stick input. Native movement retains
    // the original L-stick state immediately after this call, including menus,
    // subsequent physics and other hooks.
    s_scopes.back()={link,link->mStickValue,link->mMoveValue,link->mStickAngle,link->mMoveAngle};
    const float value=std::clamp(mDoCPd_c::getSubStickValue(PAD_1),0.f,1.f);
    link->mStickValue=link->mMoveValue=value>.05f?value:0.f;
    if(value>.05f) {
        link->mStickAngle=mDoCPd_c::getSubStickAngle(PAD_1)+0x8000;
        link->mMoveAngle=link->mStickAngle+dCam_getControledAngleY(dComIfGp_getCamera(link->field_0x317c));
    }
    return HOOK_CONTINUE;
}
void after_aim(ModContext*,void*,void*,void*) {
    if(s_scopes.empty())return;
    const auto state=s_scopes.back();s_scopes.pop_back();
    if(!state.link)return;
    state.link->mStickValue=state.stick;state.link->mMoveValue=state.move;
    state.link->mStickAngle=state.stickAngle;state.link->mMoveAngle=state.moveAngle;
}
}

ConfigVarHandle rewired_aim_movement_var(){return s_enabled;}
ModResult initialize_rewired_aim_movement(ModError* error) {
    ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;desc.name="aim-movement";
    desc.type=CONFIG_VAR_BOOL;desc.default_bool=true;
    auto result=svc_config->register_var(mod_ctx,&desc,&s_enabled);
    HookOptions options=HOOK_OPTIONS_INIT;options.priority=-1200;
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredAimStick>(svc_hook,before_aim,&options);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredAimStick>(svc_hook,after_aim,&options);
    return result==MOD_OK?MOD_OK:mods::set_error(error,result,"could not restore aim movement");
}
}
