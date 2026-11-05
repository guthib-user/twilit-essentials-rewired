#pragma once
#include "mods/api.h"
#include "mods/svc/config.h"
class daAlink_c;
namespace dawnlight {
ModResult initialize_kcrux_controls(ModError* error);
void shutdown_kcrux_controls();
bool kcrux_controls_enabled();
bool kcrux_jump_button_pressed();
int kcrux_collection_page();
int kcrux_combination_state(unsigned char inventory);
void kcrux_report_collection_page(int page);
void kcrux_assign_collection_selection(unsigned char inventory);
bool kcrux_aiming_item(daAlink_c* link);
unsigned char kcrux_cancel_action(daAlink_c* link);
unsigned char kcrux_push_action(daAlink_c* link);
ConfigVarHandle kcrux_controls_config_var();
struct KcruxSlotView { unsigned char inventory=255,mix=255,item=255; int count=-1,maximum=0; bool oil=false,available=false; };
KcruxSlotView kcrux_slot_view(int slot);
int kcrux_active_slot();
int kcrux_find_equipped_slot(const daAlink_c* link, int itemGroup);
unsigned char kcrux_resolved_item(int slot);
int kcrux_project_equipped_item(daAlink_c* link, int itemGroup);
int kcrux_item_owner(unsigned char inventory);
unsigned kcrux_assignment_revision();
bool kcrux_assign_item(int slot,unsigned char inventory);
bool kcrux_assign_item_from_menu(int slot,unsigned char inventory);
}
