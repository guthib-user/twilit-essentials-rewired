#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include <mods/service.hpp>
namespace dawnlight { ConfigVarHandle rewired_aim_movement_var(); }
namespace dawnlight { ConfigVarHandle rewired_swim_vertical_var(); }

namespace {
ModResult build(ModContext* ctx, UiElementHandle pane, void*, ModError*) {
    auto result = svc_ui->pane_add_section(ctx, pane, "Control scheme");
    if (result != MOD_OK) return result;
    UiControlDesc modern = UI_CONTROL_DESC_INIT;
    modern.kind = UI_CONTROL_TOGGLE;
    modern.label = "Modern five-slot controls";
    modern.help_rml = "On: X sword, D-pad left/right/up/down and RT items, LB map, RB items, L3 Midna. Off: Twilit Essentials' original controls and item slots. Save and restart through the Rewired launcher after changing this setting.";
    modern.binding = UI_BINDING_CONFIG_VAR;
    modern.config_var = dawnlight::kcrux_controls_config_var();
    result = svc_ui->pane_add_control(ctx, pane, &modern, nullptr);
    if (result != MOD_OK) return result;
    UiControlDesc aiming=UI_CONTROL_DESC_INIT;
    aiming.kind=UI_CONTROL_TOGGLE;aiming.label="Move while aiming";
    aiming.help_rml="In modern controls, keeps movement on the left stick and item aiming on the right stick. Scoped aiming and mounted aiming retain their native behavior.";
    aiming.binding=UI_BINDING_CONFIG_VAR;aiming.config_var=dawnlight::rewired_aim_movement_var();
    result=svc_ui->pane_add_control(ctx,pane,&aiming,nullptr);
    if(result!=MOD_OK)return result;
    UiControlDesc swimming=UI_CONTROL_DESC_INIT;
    swimming.kind=UI_CONTROL_TOGGLE;swimming.label="Reverse Zora swim up/down";
    swimming.help_rml="Reverses left-stick up/down underwater in Zora armor, including Better Swimming. Walking, surface swimming and the right-stick camera are unchanged.";
    swimming.binding=UI_BINDING_CONFIG_VAR;swimming.config_var=dawnlight::rewired_swim_vertical_var();
    result=svc_ui->pane_add_control(ctx,pane,&swimming,nullptr);
    if(result!=MOD_OK)return result;
    return svc_ui->pane_add_text(ctx, pane,
        "Changes take effect after restarting through the Rewired launcher. Rewired HUD supplies the matching on-screen layout when modern controls are on.",
        nullptr);
}
}

ModResult register_settings_ui(ModError* error) {
    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build;
    const auto result = svc_ui->register_mods_panel(mod_ctx, &panel);
    return result == MOD_OK ? MOD_OK :
        mods::set_error(error, result, "could not register Rewired settings");
}
