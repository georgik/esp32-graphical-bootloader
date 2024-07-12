#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "esp_timer.h"
#include "driver/i2s.h"

#define TAG "SynthPiano"
#define SAMPLE_RATE 44100
#define DEFAULT_VOLUME  (100)

static lv_obj_t *label;
static lv_obj_t *octave_label;
static int current_octave = 4;
static int64_t last_press_time = 0;
static esp_codec_dev_handle_t spk_codec_dev = NULL;

static void update_display() {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "Octave: %d", current_octave);
    bsp_display_lock(0);
    lv_label_set_text(octave_label, buffer);
    bsp_display_unlock();
}

static void play_tone(float frequency, int duration_ms) {
    const int sample_rate = SAMPLE_RATE;
    const int num_samples = (sample_rate * duration_ms) / 1000;
    const float amplitude = 1;

    int16_t *samples = malloc(num_samples * sizeof(int16_t));
    if (!samples) {
        ESP_LOGE(TAG, "Failed to allocate memory for samples");
        return;
    }

    for (int i = 0; i < num_samples; ++i) {
        samples[i] = (int16_t)(amplitude * INT16_MAX * sinf(2.0f * M_PI * frequency * i / sample_rate));
    }

    esp_codec_dev_open(spk_codec_dev, &(esp_codec_dev_sample_info_t){
        .sample_rate = sample_rate,
        .channel = 1,
        .bits_per_sample = 16,
    });

    esp_codec_dev_write(spk_codec_dev, samples, num_samples * sizeof(int16_t));
    vTaskDelay(pdMS_TO_TICKS(duration_ms)); // Ensure the tone plays for the specified duration

    esp_codec_dev_close(spk_codec_dev);
    free(samples);
}

static void note_event_cb(lv_event_t *e) {
    int64_t now = esp_timer_get_time();
    if (now - last_press_time < 500000) { // 500 ms debounce time
        return;
    }
    last_press_time = now;

    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btn, lv_btnmatrix_get_selected_btn(btn));

    float frequencies[12] = {
        261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88
    };
    int note_index = atoi(txt);
    if (note_index >= 0 && note_index < 12) {
        float frequency = frequencies[note_index] * powf(2.0f, current_octave - 4);
        play_tone(frequency, 500); // Play note for 500 ms
    }
}

static void octave_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btn, lv_btnmatrix_get_selected_btn(btn));

    if (code == LV_EVENT_CLICKED) {
        if (strcmp(txt, "Up") == 0) {
            current_octave = (current_octave < 8) ? current_octave + 1 : current_octave;
        } else if (strcmp(txt, "Down") == 0) {
            current_octave = (current_octave > 0) ? current_octave - 1 : current_octave;
        }
        update_display();
    }
}

void app_audio_init(void)
{
    /* Initialize speaker */
    spk_codec_dev = bsp_audio_codec_speaker_init();
    assert(spk_codec_dev);
    /* Speaker output volume */
    esp_codec_dev_set_out_vol(spk_codec_dev, DEFAULT_VOLUME);
}

void app_main(void) {
    // Initialize the BSP
    bsp_i2c_init();
    bsp_display_start();
    lv_init();
    app_audio_init();

    // Initialize Audio
    spk_codec_dev = bsp_audio_codec_speaker_init();
    assert(spk_codec_dev);
    esp_codec_dev_set_out_vol(spk_codec_dev, 50);

    // Create a label for the octave
    octave_label = lv_label_create(lv_scr_act());
    lv_label_set_text(octave_label, "Octave: 4");
    lv_obj_align(octave_label, LV_ALIGN_TOP_MID, 0, 10);

    // Create a button matrix for the notes
    static const char *note_map[] = {
        "0", "1", "2", "3", "\n",
        "4", "5", "6", "7", "\n",
        "8", "9", "10", "11", ""
    };

    lv_obj_t *note_btnm = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(note_btnm, note_map);
    lv_obj_set_size(note_btnm, 320, 150);
    lv_obj_align(note_btnm, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(note_btnm, note_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Create a button matrix for the octave control
    static const char *octave_map[] = {
        "Up", "\n",
        "Down", ""
    };

    lv_obj_t *octave_btnm = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(octave_btnm, octave_map);
    lv_obj_set_size(octave_btnm, 100, 100);
    lv_obj_align(octave_btnm, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(octave_btnm, octave_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    bsp_display_backlight_on();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
