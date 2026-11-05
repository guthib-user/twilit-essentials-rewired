#include "kcrux/controls.h"
#include "kcrux_controls.hpp"
#include <mods/service.hpp>

namespace {
uint32_t enabled() { return dawnlight::kcrux_controls_enabled(); }
uint32_t revision() { return dawnlight::kcrux_assignment_revision(); }
int32_t active_slot() { return dawnlight::kcrux_active_slot(); }
ModResult get_slot(uint32_t index, KcruxItemSlot* out) {
    if (index >= 5 || !out || out->struct_size < sizeof(KcruxItemSlot))
        return MOD_INVALID_ARGUMENT;
    const auto slot = dawnlight::kcrux_slot_view(index);
    *out = KcruxItemSlot{sizeof(KcruxItemSlot), slot.inventory, slot.mix, slot.item,
        static_cast<uint8_t>(slot.available), slot.count, slot.maximum,
        static_cast<uint8_t>(slot.oil)};
    return MOD_OK;
}
int32_t owner(uint8_t inventory) { return dawnlight::kcrux_item_owner(inventory); }
ModResult assign(uint32_t index, uint8_t inventory) {
    if (index >= 5 || inventory == 255) return MOD_INVALID_ARGUMENT;
    return dawnlight::kcrux_assign_item_from_menu(index, inventory) ? MOD_OK : MOD_UNAVAILABLE;
}
uint8_t cancel(void* link) {
    return dawnlight::kcrux_cancel_action(static_cast<daAlink_c*>(link));
}
uint8_t secondary(void* link) {
    return dawnlight::kcrux_push_action(static_cast<daAlink_c*>(link));
}
uint32_t aiming(void* link) {
    return dawnlight::kcrux_aiming_item(static_cast<daAlink_c*>(link));
}
int32_t collection_page(){return dawnlight::kcrux_collection_page();}
void report_collection_page(int32_t page){dawnlight::kcrux_report_collection_page(page);}
void assign_collection_selection(uint8_t inventory){dawnlight::kcrux_assign_collection_selection(inventory);}
int32_t combination_state(uint8_t inventory){return dawnlight::kcrux_combination_state(inventory);}
constexpr KcruxControlsService s_service{
    SERVICE_HEADER(KcruxControlsService, 1, 4),
    enabled, revision, active_slot, get_slot, owner, assign, cancel, secondary, aiming, collection_page,report_collection_page,assign_collection_selection,combination_state,
};
EXPORT_SERVICE(s_service);
}
