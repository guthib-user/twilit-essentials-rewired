#pragma once
#include "mods/svc/config.h"
#include "mods/svc/hook.h"
#include "mods/svc/log.h"
#include "mods/svc/resource.h"
#include "mods/svc/ui.h"
#include "kcrux/controls.h"
#include "twilit_essentials/stamina.h"
extern const TwilitEssentialsStaminaService* svc_te_stamina;
extern const ConfigService* svc_config;
extern const HookService* svc_hook;
extern const LogService* svc_log;
extern const ResourceService* svc_resource;
extern const UiService* svc_ui;
