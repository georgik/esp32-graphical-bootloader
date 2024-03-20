#include <math.h>
#include "lvgl.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

typedef struct {
    lv_obj_t *scr;
    int count_val;
} my_timer_context_t;

static lv_obj_t *img_text;
static lv_obj_t *label_value; // Label to display the value

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
    lv_obj_align_to(flex_cont, ref_obj, LV_ALIGN_OUT_TOP_MID, 0, -10);
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

static void add_icon_buttons(lv_obj_t *scr, lv_obj_t *ref_obj) {
    int button_width = 60;  // Button width
    int button_height = 60; // Button height

    // Create a flex container for the icon buttons
    lv_obj_t *icon_flex_cont = lv_obj_create(scr);
    lv_obj_set_size(icon_flex_cont, 4 * button_width + 3 * 10, button_height);
    lv_obj_align_to(icon_flex_cont, ref_obj, LV_ALIGN_OUT_TOP_MID, 0, -button_height - 20);
    lv_obj_set_flex_flow(icon_flex_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(icon_flex_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scrollbar_mode(icon_flex_cont, LV_SCROLLBAR_MODE_OFF);

    // Array of raw images for icons
    const lv_img_dsc_t *icons[] = {&cloud_raw, &sun_raw, &snowflake_raw, &wind_raw};

    // Create and position the buttons within the icon flex container
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(icon_flex_cont);
        lv_obj_set_size(btn, button_width, button_height);

        // Create an image for the button with the icon
        lv_obj_t *img = lv_img_create(btn);
        lv_img_set_src(img, icons[i]);
        lv_obj_center(img);  // Center the image within the button
    }
}


void bootloader_ui(lv_obj_t *scr) {
    // Create new image and make it transparent
    img_text = lv_img_create(scr);
    lv_img_set_src(img_text, &esp_text);
    lv_obj_set_style_img_opa(img_text, 0, 0);

    // Create a label for displaying the slider value
    label_value = lv_label_create(scr);
    lv_label_set_text(label_value, "0°C"); // Initialize with a default value

    // Create a slider
    slider = lv_slider_create(scr);
    lv_obj_set_width(slider, 200); // Set the slider's width

    // Position the slider at the bottom of the screen
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, -10); // -10 pixels from the bottom for some padding

    // Position the label above the slider
    lv_obj_align_to(label_value, slider, LV_ALIGN_OUT_TOP_MID, 0, -10); // Align above slider with some padding

    // Set the slider's range
    lv_slider_set_range(slider, -20, 40);

    // Add event callback to the slider
    lv_obj_add_event_cb(slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    add_number_buttons(scr, label_value);
    add_icon_buttons(scr, label_value);
}
