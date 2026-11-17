#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include "d/actor/d_a_alink.h"
#include "d/d_com_inf_game.h"
#include "d/d_menu_fmap2D.h"
#include "d/d_meter2_info.h"
#include "dusk/config_var.hpp"
#include "mods/svc/hook.hpp"
#include <string_view>

// The public config service scopes reads to a mod's own variables.
namespace dusk::config {
ConfigVarBase* GetConfigVar(std::string_view name);
}

namespace dawnlight {
namespace {
DEFINE_HOOK(&dMenu_Fmap2DTop_c::checkPlayerWarpAccept, RewiredMapWarpEligibility);
DEFINE_HOOK(&dComIfGs_isEventBit, RewiredShadowCrystalCheck);

constexpr u16 ShadowCrystal = 0x0D04;
thread_local unsigned s_warpEligibilityDepth = 0;

HookAction before_eligibility(ModContext*, void*, void*, void*) {
    ++s_warpEligibilityDepth;
    return HOOK_CONTINUE;
}

void after_eligibility(ModContext*, void*, void*, void*) {
    if (s_warpEligibilityDepth) --s_warpEligibilityDepth;
}

void after_event_bit(ModContext*, void* args, void* result, void*) {
    if (!s_warpEligibilityDepth || *static_cast<BOOL*>(result) ||
        mods::arg<u16>(args, 0) != ShadowCrystal || !kcrux_controls_enabled() ||
        dMeter2Info_getWindowStatus() != 4)
        return;

    auto* link = daAlink_getAlinkActorClass();
    if (!link || link->checkWolf()) return;

    const auto* setting = dynamic_cast<const dusk::config::ConfigVar<bool>*>(
        dusk::config::GetConfigVar("mod.com_dusklight_twilit__essentials.generalHumanWarpAnimation"));
    // Only this map eligibility query sees the Shadow Crystal requirement
    // satisfied. The native portal, terrain, and story checks still run.
    if (setting && setting->getValue()) *static_cast<BOOL*>(result) = TRUE;
}
}

ModResult initialize_rewired_map_warp(ModError* error) {
    auto result = mods::hook::add_pre<RewiredMapWarpEligibility>(svc_hook, before_eligibility);
    if (result == MOD_OK)
        result = mods::hook::add_post<RewiredMapWarpEligibility>(svc_hook, after_eligibility);
    if (result == MOD_OK)
        result = mods::hook::add_post<RewiredShadowCrystalCheck>(svc_hook, after_event_bit);
    return result == MOD_OK ? MOD_OK : mods::set_error(error, result, "could not install human map warp");
}
}
