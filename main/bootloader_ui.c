#include <math.h>
#include <sys/time.h>
#include "lvgl.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"

typedef struct {
    lv_obj_t *scr;
    int count_val;
} my_timer_context_t;

typedef struct {
    lv_style_t style;
    lv_style_t style_focus_no_outline;
    lv_style_t style_focus;
    lv_style_t style_pr;
} button_style_t;

typedef struct {
    char *name;
    void *img_src;
    void (*start_fn)(void (*fn)(void));
    void (*end_fn)(void);
} item_desc_t;

static const char *TAG = "bootloader_ui";

LV_FONT_DECLARE(font_icon_16);

static int g_item_index = 0;
static lv_group_t *g_btn_op_group = NULL;
static button_style_t g_btn_styles;
static lv_obj_t *g_page_menu = NULL;
static int64_t last_btn_press_time = 0;

static lv_obj_t *g_focus_last_obj = NULL;
static lv_obj_t *g_group_list[3] = {0};

LV_IMG_DECLARE(icon_tic_tac_toe)
LV_IMG_DECLARE(icon_wifi_list)
LV_IMG_DECLARE(icon_calculator)
LV_IMG_DECLARE(icon_synth_piano)
LV_IMG_DECLARE(icon_game_of_life)

void ui_app1_start(void (*fn)(void));
void ui_app2_start(void (*fn)(void));
void ui_app3_start(void (*fn)(void));
void ui_app4_start(void (*fn)(void));
void ui_app5_start(void (*fn)(void));

static item_desc_t item[] = {
    { "Tic-Tac-Toe", (void *) &icon_tic_tac_toe, ui_app1_start, NULL},
    { "Wi-Fi List", (void *) &icon_wifi_list, ui_app2_start, NULL},
    { "Calculator", (void *) &icon_calculator, ui_app3_start, NULL},
    { "Piano", (void *) &icon_synth_piano, ui_app4_start, NULL},
    { "Game of Life", (void *) &icon_game_of_life, ui_app5_start, NULL},
};

static lv_obj_t *g_img_btn, *g_img_item = NULL;
static lv_obj_t *g_lab_item = NULL;
static lv_obj_t *g_led_item[6];
static size_t g_item_size = sizeof(item) / sizeof(item[0]);
static lv_obj_t *g_status_bar = NULL;

static uint32_t menu_get_num_offset(uint32_t focus, int32_t max, int32_t offset)
{
    if (focus >= max) {
        ESP_LOGI(TAG, "[ERROR] focus should less than max");
        return focus;
    }

    uint32_t i;
    if (offset >= 0) {
        i = (focus + offset) % max;
    } else {
        offset = max + (offset % max);
        i = (focus + offset) % max;
    }
    return i;
}


lv_obj_t *ui_main_get_status_bar(void)
{
    return g_status_bar;
}

button_style_t *ui_button_styles(void)
{
    return &g_btn_styles;
}

lv_group_t *ui_get_btn_op_group(void)
{
    return g_btn_op_group;
}

static void ui_button_style_init(void)
{
    /*Init the style for the default state*/

    lv_style_init(&g_btn_styles.style);

    lv_style_set_radius(&g_btn_styles.style, 5);

    lv_style_set_bg_color(&g_btn_styles.style, lv_color_make(255, 255, 255));

    lv_style_set_border_opa(&g_btn_styles.style, LV_OPA_30);
    lv_style_set_border_width(&g_btn_styles.style, 2);
    lv_style_set_border_color(&g_btn_styles.style, lv_palette_main(LV_PALETTE_GREY));

    lv_style_set_shadow_width(&g_btn_styles.style, 7);
    lv_style_set_shadow_color(&g_btn_styles.style, lv_color_make(0, 0, 0));
    lv_style_set_shadow_ofs_x(&g_btn_styles.style, 0);
    lv_style_set_shadow_ofs_y(&g_btn_styles.style, 0);

    /*Init the pressed style*/

    lv_style_init(&g_btn_styles.style_pr);

    lv_style_set_border_opa(&g_btn_styles.style_pr, LV_OPA_40);
    lv_style_set_border_width(&g_btn_styles.style_pr, 2);
    lv_style_set_border_color(&g_btn_styles.style_pr, lv_palette_main(LV_PALETTE_GREY));


    lv_style_init(&g_btn_styles.style_focus);
    lv_style_set_outline_color(&g_btn_styles.style_focus, lv_color_make(255, 0, 0));

    lv_style_init(&g_btn_styles.style_focus_no_outline);
    lv_style_set_outline_width(&g_btn_styles.style_focus_no_outline, 0);
}

static int8_t menu_direct_probe(lv_obj_t *focus_obj)
{
    int8_t direct;
    uint32_t index_max_sz, index_focus, index_prev;

    index_focus = 0;
    index_prev = 0;
    index_max_sz = sizeof(g_group_list) / sizeof(g_group_list[0]);

    for (int i = 0; i < index_max_sz; i++) {
        if (focus_obj == g_group_list[i]) {
            index_focus = i;
        }
        if (g_focus_last_obj == g_group_list[i]) {
            index_prev = i;
        }
    }

    if (NULL == g_focus_last_obj) {
        direct = 0;
    } else if (index_focus == menu_get_num_offset(index_prev, index_max_sz, 1)) {
        direct = 1;
    } else if (index_focus == menu_get_num_offset(index_prev, index_max_sz, -1)) {
        direct = -1;
    } else {
        direct = 0;
    }

    g_focus_last_obj = focus_obj;
    return direct;
}

static void menu_prev_cb(lv_event_t *e)
{
    bsp_display_lock(0);
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_RELEASED == code) {
        int64_t now = esp_timer_get_time();
        if (now - last_btn_press_time < 500000) { // 500 ms debounce time
            bsp_display_unlock();
            return;
        }
        last_btn_press_time = now;

        lv_led_off(g_led_item[g_item_index]);
        if (0 == g_item_index) {
            g_item_index = g_item_size;
        }
        g_item_index--;
        lv_led_on(g_led_item[g_item_index]);
        lv_img_set_src(g_img_item, item[g_item_index].img_src);
        lv_label_set_text_static(g_lab_item, item[g_item_index].name);
    }
    bsp_display_unlock();
}

static void menu_next_cb(lv_event_t *e)
{
    bsp_display_lock(0);
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_RELEASED == code) {
        int64_t now = esp_timer_get_time();
        if (now - last_btn_press_time < 500000) { // 500 ms debounce time
            bsp_display_unlock();
            return;
        }
        last_btn_press_time = now;

        lv_led_off(g_led_item[g_item_index]);
        g_item_index++;
        if (g_item_index >= g_item_size) {
            g_item_index = 0;
        }
        lv_led_on(g_led_item[g_item_index]);
        lv_img_set_src(g_img_item, item[g_item_index].img_src);
        lv_label_set_text_static(g_lab_item, item[g_item_index].name);
    }
    bsp_display_unlock();
}

static void ui_led_set_visible(bool visible)
{
    bsp_display_lock(0);
    for (size_t i = 0; i < sizeof(g_led_item) / sizeof(g_led_item[0]); i++) {
        if (NULL != g_led_item[i]) {
            if (visible) {
                lv_obj_clear_flag(g_led_item[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(g_led_item[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    bsp_display_unlock();
}


void menu_new_item_select(lv_obj_t *obj)
{
    bsp_display_lock(0);
    int8_t direct = menu_direct_probe(obj);
    g_item_index = menu_get_num_offset(g_item_index, g_item_size, direct);

    lv_led_on(g_led_item[g_item_index]);
    lv_img_set_src(g_img_item, item[g_item_index].img_src);
    lv_label_set_text_static(g_lab_item, item[g_item_index].name);
    bsp_display_unlock();
}

static void menu_enter_cb(lv_event_t *e)
{
    bsp_display_lock(0);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_user_data(e);

    if (LV_EVENT_FOCUSED == code) {
        lv_led_off(g_led_item[g_item_index]);
        menu_new_item_select(obj);
    } else if (LV_EVENT_CLICKED == code) {
        lv_obj_t *menu_btn_parent = lv_obj_get_parent(obj);
        ESP_LOGI(TAG, "menu click, item index = %d", g_item_index);
        if (ui_get_btn_op_group()) {
            lv_group_remove_all_objs(ui_get_btn_op_group());
        }
// #if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
//         bsp_btn_rm_all_callback(BSP_BUTTON_MAIN);
// #endif
        ui_led_set_visible(false);
        lv_obj_del(menu_btn_parent);
        g_focus_last_obj = NULL;

        item[g_item_index].start_fn(item[g_item_index].end_fn);
    }
    bsp_display_unlock();
}

static void ui_main_menu(int32_t index_id)
{
    if (!g_page_menu) {
        g_page_menu = lv_obj_create(lv_scr_act());
        lv_obj_set_size(g_page_menu, lv_obj_get_width(lv_obj_get_parent(g_page_menu)), lv_obj_get_height(lv_obj_get_parent(g_page_menu)) - lv_obj_get_height(ui_main_get_status_bar()));
        lv_obj_set_style_border_width(g_page_menu, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(g_page_menu, lv_obj_get_style_bg_color(lv_scr_act(), LV_STATE_DEFAULT), LV_PART_MAIN);
        lv_obj_clear_flag(g_page_menu, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align_to(g_page_menu, ui_main_get_status_bar(), LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    }

    lv_obj_t *obj = lv_obj_create(g_page_menu);
    lv_obj_set_size(obj, 290, 174);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, 15, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_30, LV_PART_MAIN);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, -10);

    g_img_btn = lv_btn_create(obj);
    lv_obj_set_size(g_img_btn, 108, 108);
    lv_obj_add_style(g_img_btn, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(g_img_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(g_img_btn, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(g_img_btn, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(g_img_btn, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_img_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(g_img_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(g_img_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(g_img_btn, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_radius(g_img_btn, 40, LV_PART_MAIN);
    lv_obj_align(g_img_btn, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_event_cb(g_img_btn, menu_enter_cb, LV_EVENT_ALL, g_img_btn);

    g_img_item = lv_img_create(g_img_btn);
    lv_img_set_src(g_img_item, item[index_id].img_src);
    lv_obj_center(g_img_item);

    g_lab_item = lv_label_create(obj);
    lv_label_set_text_static(g_lab_item, item[index_id].name);
    lv_obj_set_style_text_font(g_lab_item, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_align(g_lab_item, LV_ALIGN_CENTER, 0, 60);

    for (size_t i = 0; i < sizeof(g_led_item) / sizeof(g_led_item[0]); i++) {
        int gap = 10;
        if (NULL == g_led_item[i]) {
            g_led_item[i] = lv_led_create(g_page_menu);
        } else {
            lv_obj_clear_flag(g_led_item[i], LV_OBJ_FLAG_HIDDEN);
        }
        lv_led_off(g_led_item[i]);
        lv_obj_set_size(g_led_item[i], 5, 5);
        lv_obj_align_to(g_led_item[i], g_page_menu, LV_ALIGN_BOTTOM_MID, 2 * gap * i - (g_item_size - 1) * gap, 0);
    }
    lv_led_on(g_led_item[index_id]);

    lv_obj_t *btn_prev = lv_btn_create(obj);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_prev, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);

    lv_obj_set_size(btn_prev, 40, 40);
    lv_obj_set_style_bg_color(btn_prev, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn_prev, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_prev, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn_prev, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(btn_prev, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn_prev, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_prev, 20, LV_PART_MAIN);
    lv_obj_align_to(btn_prev, obj, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *label = lv_label_create(btn_prev);
    lv_label_set_text_static(label, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(label, lv_color_make(5, 5, 5), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_prev, menu_prev_cb, LV_EVENT_ALL, btn_prev);


    lv_obj_t *btn_next = lv_btn_create(obj);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_pr, LV_STATE_PRESSED);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUS_KEY);
    lv_obj_add_style(btn_next, &ui_button_styles()->style_focus_no_outline, LV_STATE_FOCUSED);

    lv_obj_set_size(btn_next, 40, 40);
    lv_obj_set_style_bg_color(btn_next, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn_next, lv_color_make(0, 0, 0), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn_next, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn_next, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_x(btn_next, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(btn_next, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_next, 20, LV_PART_MAIN);
    lv_obj_align_to(btn_next, obj, LV_ALIGN_RIGHT_MID, 0, 0);
    label = lv_label_create(btn_next);
    lv_label_set_text_static(label, LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(label, lv_color_make(5, 5, 5), LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_next, menu_next_cb, LV_EVENT_ALL, btn_next);
}


static void ota_swich_to_app(int app_index) {
    // Initially assume the first OTA partition, which is typically 'ota_0'
    const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);

    // Iterate to find the correct OTA partition only if button ID is greater than 1
    if (app_index > 0 && app_index <= 5) {
        for (int i = 0; i < app_index; i++) {
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



void ui_app1_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "App1 start");
    ota_swich_to_app(0);
}

void ui_app2_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "App2 start");
    ota_swich_to_app(1);
}

void ui_app3_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "App3 start");
    ota_swich_to_app(2);
}

void ui_app4_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "App4 start");
    ota_swich_to_app(3);
}

void ui_app5_start(void (*fn)(void))
{
    ESP_LOGI(TAG, "App5 start");
    ota_swich_to_app(4);
}

void bootloader_ui(lv_obj_t *scr) {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(237, 238, 239), LV_STATE_DEFAULT);
    ui_button_style_init();

    lv_indev_t *indev = lv_indev_get_next(NULL);

    if ((lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) || \
            lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
        ESP_LOGI(TAG, "Input device type is keypad");
        g_btn_op_group = lv_group_create();
        lv_indev_set_group(indev, g_btn_op_group);
    } else if (lv_indev_get_type(indev) == LV_INDEV_TYPE_BUTTON) {
        ESP_LOGI(TAG, "Input device type have button");
    } else if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
        ESP_LOGI(TAG, "Input device type have pointer");
    }

    // Create status bar
    g_status_bar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(g_status_bar, lv_obj_get_width(lv_obj_get_parent(g_status_bar)), 0);
    lv_obj_clear_flag(g_status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(g_status_bar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(g_status_bar, lv_obj_get_style_bg_color(lv_scr_act(), LV_STATE_DEFAULT), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(g_status_bar, 0, LV_PART_MAIN);
    lv_obj_align(g_status_bar, LV_ALIGN_TOP_MID, 0, 0);

    ui_main_menu(g_item_index);
}
