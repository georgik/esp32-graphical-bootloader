#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_ota_ops.h"

#define TAG "WiFiList"
#define DEFAULT_SCAN_LIST_SIZE 32

static lv_obj_t *list;
static bool scan_in_progress = false;
static lv_obj_t *search_msg_box = NULL;

static void list_wifi();
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void close_message_box() {
    if (search_msg_box) {
        bsp_display_lock(0);
        lv_msgbox_close(search_msg_box);
        bsp_display_unlock();
        search_msg_box = NULL;
    }
}

static void show_message(const char *text) {
    ESP_LOGI(TAG, "%s", text);
    bsp_display_lock(0);
    search_msg_box = lv_msgbox_create(NULL);
    lv_msgbox_add_text(search_msg_box, text);
    lv_obj_center(search_msg_box);
    bsp_display_unlock();
}

void list_wifi() {
    if (scan_in_progress) {
        ESP_LOGW(TAG, "Scan already in progress");
        return;
    }

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,  // Disable hidden networks
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = { .active = { .min = 500, .max = 1500 } }  // Increased scan times
    };

    esp_err_t err = esp_wifi_scan_start(&scan_config, false);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(err));
        return;
    }

    scan_in_progress = true;
}

void handle_scan_done() {
    uint16_t ap_count = 0;

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    printf("Found %d access points\n", ap_count);

    wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_records == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for AP records");
        scan_in_progress = false;
        return;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
    printf("Scan done\n");

    bsp_display_lock(0);
    // Clear the current list items
    while (lv_obj_get_child_cnt(list) > 0) {
        lv_obj_del(lv_obj_get_child(list, 0));
    }

    // Add new list items
    for (int i = 0; i < ap_count; i++) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%s (%d)", ap_records[i].ssid, ap_records[i].rssi);
        lv_list_add_text(list, buffer);
    }
    bsp_display_unlock();

    free(ap_records);
    scan_in_progress = false;

    close_message_box();  // Close the "Searching..." message box after scan is done
}

void reset_to_factory_app() {
    // Get the partition structure for the factory partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition != NULL) {
        if (esp_ota_set_boot_partition(factory_partition) == ESP_OK) {
            printf("Set boot partition to factory, restarting now.\n");
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

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Initialize the event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_SCAN_DONE, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Create a label for WiFi list title
    bsp_display_lock(0);
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "List of Wi-Fi networks");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    // Create a list for WiFi networks
    list = lv_list_create(lv_scr_act());
    lv_obj_set_size(list, 300, 180);  // Adjust height to leave space for the label
    lv_obj_align(list, LV_ALIGN_CENTER, 0, 20);
    bsp_display_unlock();

    bsp_display_backlight_on();

    show_message("Searching...");  // Show the "Searching..." message at the start

    while (1) {
        list_wifi();
        vTaskDelay(pdMS_TO_TICKS(60000)); // Refresh every 60 seconds
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
        handle_scan_done();
    }
}
