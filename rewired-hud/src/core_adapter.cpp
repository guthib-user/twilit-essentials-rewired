#include "kcrux_controls.hpp"
#include "service_imports.hpp"

namespace dawnlight {
bool kcrux_controls_enabled() {
    return svc_kcrux_controls && svc_kcrux_controls->enabled();
}
int kcrux_collection_page(){return svc_kcrux_controls?svc_kcrux_controls->collection_page():-1;}
bool kcrux_aiming_item(daAlink_c* link) {
    return svc_kcrux_controls && svc_kcrux_controls->aiming_item(link);
}
unsigned char kcrux_cancel_action(daAlink_c* link) {
    return svc_kcrux_controls ? svc_kcrux_controls->cancel_action(link) : 0;
}
unsigned char kcrux_push_action(daAlink_c* link) {
    return svc_kcrux_controls ? svc_kcrux_controls->secondary_action(link) : 0;
}
KcruxSlotView kcrux_slot_view(int slot) {
    KcruxSlotView out;
    if (!svc_kcrux_controls || slot < 0 || slot >= 5) return out;
    KcruxItemSlot value = KCRUX_ITEM_SLOT_INIT;
    if (svc_kcrux_controls->get_slot(slot, &value) != MOD_OK) return out;
    out.inventory = value.inventory;
    out.mix = value.mix;
    out.item = value.resolved_item;
    out.available = value.available;
    out.count = value.count;
    out.maximum = value.maximum;
    out.oil = value.uses_oil;
    return out;
}
int kcrux_active_slot() {
    return svc_kcrux_controls ? svc_kcrux_controls->active_slot() : -1;
}
int kcrux_item_owner(unsigned char inventory) {
    return svc_kcrux_controls ? svc_kcrux_controls->owner_of_inventory(inventory) : -1;
}
}
