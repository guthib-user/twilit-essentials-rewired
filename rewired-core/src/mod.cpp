#include "kcrux_controls.hpp"
#include "service_imports.hpp"
#include "mods/service.hpp"
#ifdef KCRUX_TEST_DRIVER
#include "kcrux_test_driver.hpp"
#endif
DEFINE_MOD();
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_OPTIONAL_SERVICE(GameModeService, svc_game_mode);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(SaveService, svc_save);
IMPORT_SERVICE(UiService, svc_ui);
#ifdef KCRUX_TEST_DRIVER
IMPORT_SERVICE(ActorService, svc_actor);
IMPORT_SERVICE(GfxService, svc_gfx);
#endif
ModResult register_settings_ui(ModError* error);
ModResult initialize_rewired_crash_compat(ModError* error);
namespace dawnlight { ModResult initialize_rewired_jump(ModError* error); void shutdown_rewired_jump(); }
namespace dawnlight { ModResult initialize_rewired_aim_movement(ModError* error); }
namespace dawnlight { ModResult initialize_rewired_swim_controls(ModError* error); }
extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    if (auto result = initialize_rewired_crash_compat(error); result != MOD_OK) return result;
    if (auto result = dawnlight::initialize_kcrux_controls(error); result != MOD_OK) return result;
    if (auto result = dawnlight::initialize_rewired_jump(error); result != MOD_OK) return result;
    if (auto result = dawnlight::initialize_rewired_aim_movement(error); result != MOD_OK) return result;
    if (auto result = dawnlight::initialize_rewired_swim_controls(error); result != MOD_OK) return result;
    if (auto result = register_settings_ui(error); result != MOD_OK) return result;
#ifdef KCRUX_TEST_DRIVER
    return dawnlight::initialize_kcrux_test_driver(error);
#else
    return MOD_OK;
#endif
}
MOD_EXPORT ModResult mod_update(ModError*) {
#ifdef KCRUX_TEST_DRIVER
    dawnlight::update_kcrux_test_driver();
#endif
    return MOD_OK;
}
MOD_EXPORT ModResult mod_shutdown(ModError*) {
#ifdef KCRUX_TEST_DRIVER
    dawnlight::shutdown_kcrux_test_driver();
#endif
    dawnlight::shutdown_rewired_jump();
    dawnlight::shutdown_kcrux_controls(); return MOD_OK;
}
}
