#pragma once

#include <mods/api.h>
#include <mods/service.hpp>
#include <stdint.h>

#define KCRUX_CONTROLS_SERVICE_ID "local.twilit_rewired.controls.slots"
#define KCRUX_CONTROLS_SERVICE_MAJOR 1u
#define KCRUX_CONTROLS_SERVICE_MINOR 4u

/* The five canonical slots are D-pad left, right, up, down, then RT. */
typedef struct KcruxItemSlot {
    uint32_t struct_size;
    uint8_t inventory;
    uint8_t mix;
    uint8_t resolved_item;
    uint8_t available;
    int32_t count;
    int32_t maximum;
    uint8_t uses_oil;
} KcruxItemSlot;

#define KCRUX_ITEM_SLOT_INIT {sizeof(KcruxItemSlot), 255, 255, 255, 0, -1, 0, 0}

typedef struct KcruxControlsService {
    ServiceHeader header;
    uint32_t (*enabled)(void);
    uint32_t (*assignment_revision)(void);
    int32_t (*active_slot)(void);
    ModResult (*get_slot)(uint32_t index, KcruxItemSlot* out_slot);
    int32_t (*owner_of_inventory)(uint8_t inventory);
    ModResult (*assign)(uint32_t index, uint8_t inventory);
    uint8_t (*cancel_action)(void* link);
    uint8_t (*secondary_action)(void* link);
    uint32_t (*aiming_item)(void* link);
    int32_t (*collection_page)(void); /* 0..4 for TE's Advanced Collection, -1 otherwise */
    void (*report_collection_page)(int32_t page); /* Actual rendered page, never inferred from button presses. */
    void (*assign_collection_selection)(uint8_t inventory);
    int32_t (*combination_state)(uint8_t inventory); /* 0 unavailable, 1 combine, 2 separate */
} KcruxControlsService;

MOD_DECLARE_SERVICE(KcruxControlsService, svc_kcrux_controls,
    KCRUX_CONTROLS_SERVICE_ID, KCRUX_CONTROLS_SERVICE_MAJOR, KCRUX_CONTROLS_SERVICE_MINOR);
