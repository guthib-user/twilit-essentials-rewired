#pragma once
#include "mods/svc/config.h"
#ifdef KCRUX_TEST_DRIVER
#include "mods/svc/actor.h"
#include "mods/svc/gfx.h"
#endif
#include "mods/svc/game_mode.h"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/save.h"
#include "mods/svc/ui.h"
extern const ConfigService* svc_config;
#ifdef KCRUX_TEST_DRIVER
extern const ActorService* svc_actor;
extern const GfxService* svc_gfx;
#endif
extern const GameModeService* svc_game_mode;
extern const HookService* svc_hook;
extern const LogService* svc_log;
extern const SaveService* svc_save;
extern const UiService* svc_ui;
