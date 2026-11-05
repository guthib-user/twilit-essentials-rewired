#pragma once
#include "mods/api.h"
namespace dawnlight {
ModResult register_hud_config(ModError*);
int kcrux_frame_offset_x();
int kcrux_frame_offset_y();
int kcrux_frame_scale_percent();
bool r_jump_enabled();
}
