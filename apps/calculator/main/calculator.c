#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"

#define TAG "Calculator"

static lv_obj_t *display_label;
static char current_input[64] = "";
static double stored_value = 0;
static char current_operator = 0;
static bool clear_next = false;
static int64_t last_press_time = 0;

static void update_display() {
    bsp_display_lock(0);
    lv_label_set_text(display_label, current_input);
    bsp_display_unlock();
}

static void clear_input() {
    current_input[0] = '\0';
    update_display();
}

static void append_input(const char *text) {
    if (clear_next) {
        clear_input();
        clear_next = false;
    }
    if (strlen(current_input) < sizeof(current_input) - 1) {
        strcat(current_input, text);
    }
    update_display();
}

static void handle_operator(char operator) {
    if (current_operator != 0 && !clear_next) {
        double current_value = atof(current_input);
        switch (current_operator) {
            case '+': stored_value += current_value; break;
            case '-': stored_value -= current_value; break;
            case '*': stored_value *= current_value; break;
            case '/': if (current_value != 0) stored_value /= current_value; break;
        }
        snprintf(current_input, sizeof(current_input), "%g", stored_value);
        update_display();
    } else {
        stored_value = atof(current_input);
    }
    current_operator = operator;
    clear_next = true;
}

static void btn_event_cb(lv_event_t *e) {
    int64_t now = esp_timer_get_time();
    if (now - last_press_time < 500000) { // 500 ms debounce time
        return;
    }
    last_press_time = now;

    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btn, lv_btnmatrix_get_selected_btn(btn));

    if (strcmp(txt, "C") == 0) {
        clear_input();
        stored_value = 0;
        current_operator = 0;
    } else if (strcmp(txt, "=") == 0) {
        handle_operator(0);
        current_operator = 0;
        clear_next = true;
    } else if (strchr("+-*/", txt[0]) != NULL) {
        handle_operator(txt[0]);
    } else {
        append_input(txt);
    }
}

void reset_to_factory_app() {
    // Get the partition structure for the factory partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition != NULL) {
        if (esp_ota_set_boot_partition(factory_partition) == ESP_OK) {
            printf("Set boot partition to factory.\n");
        } else {
            printf("Failed to set boot partition to factory.\n");
        }
    } else {
        printf("Factory partition not found.\n");
    }

    fflush(stdout);
}

void app_main(void) {
    // Reset to factory app for the next boot.
    // It should return to graphical bootloader.
    reset_to_factory_app();

    // Initialize the BSP
    bsp_i2c_init();
    bsp_display_start();
    lv_init();

    // Create a label for the display
    display_label = lv_label_create(lv_scr_act());
    lv_obj_set_size(display_label, 300, 30);
    lv_obj_align(display_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(display_label, "0");

    // Create a button matrix for the calculator
    static const char *btn_map[] = {
        "7", "8", "9", "C", "\n",
        "4", "5", "6", "*", "\n",
        "1", "2", "3", "-", "\n",
        "0", ".", "/", "+", "\n",
        "=", ""
    };

    lv_obj_t *btnm = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(btnm, btn_map);
    lv_obj_set_size(btnm, 320, 180);
    lv_obj_align(btnm, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(btnm, btn_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    bsp_display_backlight_on();
}
