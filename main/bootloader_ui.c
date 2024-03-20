#include <math.h>
#include "lvgl.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

typedef struct {
    lv_obj_t *scr;
    int count_val;
} my_timer_context_t;


static void ota_button_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const uint32_t btn_id = lv_obj_get_child_id(btn);

    printf("Button %ld clicked\n", btn_id + 1);

    // Initially assume the first OTA partition, which is typically 'ota_0'
    const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);

    // Iterate to find the correct OTA partition only if button ID is greater than 1
    if (btn_id > 0 && btn_id <= 5) {
        for (int i = 0; i < btn_id; i++) {
            next_partition = esp_ota_get_next_update_partition(next_partition);
            if (!next_partition) break;  // If no next partition, break from the loop
        }
    }

    // For button 1, next_partition will not change, thus pointing to 'ota_0'
    if (next_partition && esp_ota_set_boot_partition(next_partition) == ESP_OK) {
        printf("Setting boot partition to %s\n", next_partition->label);
        esp_restart();  // Restart to boot from the new partition
    } else {
        printf("Failed to set boot partition\n");
    }
}

static void add_number_buttons(lv_obj_t *scr, lv_obj_t *ref_obj) {
    int button_width = 50;
    int button_height = 60;

    lv_obj_t *flex_cont = lv_obj_create(scr);
    lv_obj_set_size(flex_cont, 5 * button_width + 4 * 10, button_height);

    // Check if ref_obj is provided, if not, align to the screen
    if (ref_obj) {
        lv_obj_align_to(flex_cont, ref_obj, LV_ALIGN_OUT_TOP_MID, 0, -10);
    } else {
        lv_obj_align(flex_cont, LV_ALIGN_CENTER, 0, 0); // Center the container on the screen
    }

    lv_obj_set_flex_flow(flex_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(flex_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(flex_cont, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_btn_create(flex_cont);
        lv_obj_set_size(btn, button_width, button_height);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%d", i + 1);

        lv_color_t text_color;
        switch (i) {
            case 0: text_color = lv_color_make(0, 255, 0); break;
            case 1: text_color = lv_color_make(255, 255, 0); break;
            case 2: text_color = lv_color_make(255, 165, 0); break;
            case 3: text_color = lv_color_make(255, 69, 0); break;
            case 4: text_color = lv_color_make(255, 0, 0); break;
            default: text_color = lv_color_make(0, 0, 0); break;
        }
        lv_obj_set_style_text_color(label, text_color, 0);

        // Register the event handler for the button
        lv_obj_add_event_cb(btn, ota_button_event_handler, LV_EVENT_CLICKED, NULL);
    }
}


void bootloader_ui(lv_obj_t *scr) {

    add_number_buttons(scr, NULL);
}
