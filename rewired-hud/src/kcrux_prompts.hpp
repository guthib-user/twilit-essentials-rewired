#pragma once
#include "mods/api.h"
namespace dawnlight {
ModResult initialize_kcrux_prompts(ModError*);
void shutdown_kcrux_prompts();
// Final native minimap draw bounds, including the user's scale and slide.
void kcrux_minimap_layout(float top,bool visible);
}
