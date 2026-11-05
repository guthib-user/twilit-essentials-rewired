#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_meter2_info.h"
#include "m_Do/m_Do_controller_pad.h"
#include "mods/svc/hook.hpp"
#include <algorithm>

namespace dawnlight {
namespace {
DEFINE_HOOK(&mDoCPd_c::read, RewiredSwimRead);
DEFINE_HOOK_SYMBOL("SDL_GetGamepadAxis", short(SDL_Gamepad*,int), RewiredSwimAxis);
ConfigVarHandle s_enabled=0;
thread_local SDL_Gamepad* s_swimPad=nullptr;

HookAction before_read(ModContext*,void*,void*,void*) {
    s_swimPad=nullptr;
    auto* link=daAlink_getAlinkActorClass();
    bool enabled=false,visible=true;
    if(!kcrux_controls_enabled() || !link || link->checkWolf() ||
       !link->checkZoraWearAbility() || link->checkSwimUp() ||
       link->checkEventRun() || dComIfGp_event_runCheck() ||
       dComIfGp_isPauseFlag() || dMeter2Info_getWindowStatus()!=0 ||
       svc_ui->is_any_document_visible(mod_ctx,&visible)!=MOD_OK || visible ||
       svc_config->get_bool(mod_ctx,s_enabled,&enabled)!=MOD_OK || !enabled)
        return HOOK_CONTINUE;
    switch(link->mProcID) {
    case daAlink_c::PROC_SWIM_UP:
    case daAlink_c::PROC_SWIM_WAIT:
    case daAlink_c::PROC_SWIM_MOVE:
    case daAlink_c::PROC_SWIM_DIVE:break;
    default:return HOOK_CONTINUE;
    }
    const int index=PADGetIndexForPort(PAD_1);
    if(index>=0)s_swimPad=PADGetSDLGamepadForIndex(index);
    return HOOK_CONTINUE;
}
void after_axis(ModContext*,void* args,void* result,void*) {
    if(!s_swimPad || mods::arg<SDL_Gamepad*>(args,0)!=s_swimPad ||
       mods::arg<int>(args,1)!=1)return;
    auto& value=*static_cast<short*>(result);
    value=static_cast<short>(std::clamp(-static_cast<int>(value),-32768,32767));
}
void after_read(ModContext*,void*,void*,void*) {s_swimPad=nullptr;}
}

ConfigVarHandle rewired_swim_vertical_var(){return s_enabled;}
ModResult initialize_rewired_swim_controls(ModError* error) {
    ConfigVarDesc desc=CONFIG_VAR_DESC_INIT;desc.name="reverse-zora-swim-vertical";
    desc.type=CONFIG_VAR_BOOL;desc.default_bool=true;
    auto result=svc_config->register_var(mod_ctx,&desc,&s_enabled);
    HookOptions options=HOOK_OPTIONS_INIT;options.priority=-2000;
    // Reverse once, before native pad normalization. Both Essentials' Better
    // Swimming and the native swimming code receive the same coherent stick
    // position, magnitude and angle. Other SDL polls are left untouched.
    if(result==MOD_OK)result=mods::hook::add_pre<RewiredSwimRead>(svc_hook,before_read,&options);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredSwimRead>(svc_hook,after_read,&options);
    if(result==MOD_OK)result=mods::hook::add_post<RewiredSwimAxis>(svc_hook,after_axis,&options);
    return result==MOD_OK?MOD_OK:mods::set_error(error,result,"could not register Zora swim controls");
}
}
