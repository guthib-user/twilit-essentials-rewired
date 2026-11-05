#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include <mods/service.hpp>

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
