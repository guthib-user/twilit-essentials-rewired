#include "config.hpp"
#include "kcrux_prompts.hpp"
#include "service_imports.hpp"
#include <mods/service.hpp>

DEFINE_MOD();
IMPORT_SERVICE(KcruxControlsService, svc_kcrux_controls);
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_OPTIONAL_SERVICE(TwilitEssentialsStaminaService, svc_te_stamina);

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if(auto result=dawnlight::register_hud_config(error);result!=MOD_OK)return result;
    return dawnlight::initialize_kcrux_prompts(error);
}
MOD_EXPORT ModResult mod_update(ModError*) {return MOD_OK;}
MOD_EXPORT ModResult mod_shutdown(ModError*) {
    dawnlight::shutdown_kcrux_prompts();return MOD_OK;
}
}
