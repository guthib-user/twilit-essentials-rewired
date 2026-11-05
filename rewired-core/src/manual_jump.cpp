#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include "global.h"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "mods/svc/hook.hpp"

namespace dawnlight {
namespace {
DEFINE_HOOK(&daAlink_c::checkAutoJumpAction, RewiredManualJump);

bool movement_proc(const daAlink_c* link) {
    switch (link->mProcID) {
    case daAlink_c::PROC_WAIT:
    case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE:
    case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE:
    case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN:
    case daAlink_c::PROC_WOLF_WAIT:
    case daAlink_c::PROC_WOLF_MOVE:
    case daAlink_c::PROC_WOLF_DASH:
    case daAlink_c::PROC_WOLF_WAIT_TURN:
    case daAlink_c::PROC_WOLF_ATN_AC_MOVE:
        return true;
    default:
        return false;
    }
}

bool ready(daAlink_c* link) {
    if (!link || !kcrux_jump_button_pressed() || !movement_proc(link) ||
        !link->mLinkAcch.ChkGroundHit() || link->mGndPolyAtt1 == 0xFF ||
        link->checkFlyAtnWait() || link->checkModeFlg(0x70C12) ||
        link->getSumouMode() || link->checkPlayerDemoMode() ||
        link->checkEventRun() || dComIfGp_event_runCheck() ||
        link->checkMagneBootsFly() || link->checkMagneBootsOn() ||
        link->checkNotJumpSinkLimit() || link->checkAttentionLock() ||
        link->mGrabItemAcKeep.getActor() || kcrux_push_action(link) ||
        kcrux_aiming_item(link)) return false;
    return true;
}

HookAction before_auto_jump(ModContext*, void* args, void* result, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!ready(link)) return HOOK_CONTINUE;
    // Native sidestep is a backhop or backflip. It preserves Link's facing and
    // prevents a camera snap when Y is pressed while moving backwards.
    if (!link->checkWolf() && link->checkInputOnR() &&
        link->getDirectionFromShapeAngle() == daAlink_c::DIR_BACKWARD) {
        if (!link->procSideStepInit(daAlink_c::DIR_BACKWARD)) return HOOK_CONTINUE;
    } else {
        if (link->checkInputOnR()) link->shape_angle.y = link->mMoveAngle;
        link->current.angle.y = link->shape_angle.y;
        const bool started = link->checkWolf() ? link->procWolfAutoJumpInit(1) :
            link->procAutoJumpInit(1);
        if (!started) return HOOK_CONTINUE;
        if (!link->checkInputOnR()) {
            link->speedF = 0.0f;
            link->mNormalSpeed = 0.0f;
        }
    }
    link->mLinkAcch.ClrGroundHit();
    link->setJumpMode();
    *static_cast<BOOL*>(result) = TRUE;
    return HOOK_SKIP_ORIGINAL;
}
}

ModResult initialize_rewired_jump(ModError* error) {
    HookOptions early = HOOK_OPTIONS_INIT;
    early.priority = 300;
    const auto result = mods::hook::add_pre<RewiredManualJump>(svc_hook, before_auto_jump, &early);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result, "could not install Y jump");
}
void shutdown_rewired_jump() { mods::hook::uninstall<RewiredManualJump>(svc_hook); }
}
