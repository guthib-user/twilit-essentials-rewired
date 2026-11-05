#pragma once
#include "mods/api.h"
class daAlink_c;
namespace dawnlight {
struct KcruxSlotView { unsigned char inventory=255,mix=255,item=255; int count=-1,maximum=0; bool oil=false,available=false; };
bool kcrux_controls_enabled();
int kcrux_collection_page();
bool kcrux_aiming_item(daAlink_c* link);
unsigned char kcrux_cancel_action(daAlink_c* link);
unsigned char kcrux_push_action(daAlink_c* link);
KcruxSlotView kcrux_slot_view(int slot);
int kcrux_active_slot();
int kcrux_item_owner(unsigned char inventory);
}
