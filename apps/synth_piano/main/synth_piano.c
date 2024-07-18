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
#include "esp_ota_ops.h"

#define TAG "SynthPiano"
#define SAMPLE_RATE 44100
#define DEFAULT_VOLUME  90

static lv_obj_t *octave_label;
static int current_octave = 4;
static int last_note_index = -1;
static int last_octave_event = -1;
static esp_codec_dev_handle_t spk_codec_dev = NULL;
static QueueHandle_t tone_queue;

typedef struct {
    float frequency;
    int duration_ms;
} tone_t;

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
    const float amplitude = 1.0;

    int16_t *samples = malloc(num_samples * sizeof(int16_t));
    if (!samples) {
        ESP_LOGE(TAG, "Failed to allocate memory for samples");
        return;
    }

    for (int i = 0; i < num_samples; ++i) {
        samples[i] = (int16_t)(amplitude * INT16_MAX * sinf(2.0f * M_PI * frequency * i / sample_rate));
    }

    esp_codec_dev_write(spk_codec_dev, samples, num_samples * sizeof(int16_t));
    vTaskDelay(pdMS_TO_TICKS(duration_ms)); // Ensure the tone plays for the specified duration

    free(samples);
}

static void tone_task(void *param) {
    tone_t tone;
    while (1) {
        if (xQueueReceive(tone_queue, &tone, portMAX_DELAY)) {
            play_tone(tone.frequency, tone.duration_ms);
        }
    }
}

static void note_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btn, lv_btnmatrix_get_selected_btn(btn));

    float frequencies[12] = {
        261.63, 277.18, 293.66, 311.13, 329.63, 349.23, 369.99, 392.00, 415.30, 440.00, 466.16, 493.88
    };
    int note_index = atoi(txt);
    if (note_index == last_note_index) {
        return; // Ignore repeated events for the same note
    }
    last_note_index = note_index;

    if (note_index >= 0 && note_index < 12) {
        float frequency = frequencies[note_index] * powf(2.0f, current_octave - 4);
        tone_t tone = { .frequency = frequency, .duration_ms = 250 };
        xQueueSend(tone_queue, &tone, portMAX_DELAY); // Send tone to queue
    }
}

static void octave_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *txt = lv_btnmatrix_get_btn_text(btn, lv_btnmatrix_get_selected_btn(btn));

    if ((last_octave_event == 0 && strcmp(txt, "Up") == 0) || (last_octave_event == 1 && strcmp(txt, "Down") == 0)) {
        return; // Ignore repeated events for the same octave button
    }

    if (strcmp(txt, "Up") == 0) {
        last_octave_event = 0;
        current_octave = (current_octave < 8) ? current_octave + 1 : current_octave;
    } else if (strcmp(txt, "Down") == 0) {
        last_octave_event = 1;
        current_octave = (current_octave > 0) ? current_octave - 1 : current_octave;
    }
    update_display();
}

void app_audio_init(void)
{
    /* Initialize speaker */
    spk_codec_dev = bsp_audio_codec_speaker_init();
    assert(spk_codec_dev);
    /* Speaker output volume */
    esp_codec_dev_set_out_vol(spk_codec_dev, DEFAULT_VOLUME);

    esp_codec_dev_open(spk_codec_dev, &(esp_codec_dev_sample_info_t){
        .sample_rate = SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    });
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
    app_audio_init();

    // Create a label for the octave
    octave_label = lv_label_create(lv_scr_act());
    lv_label_set_text(octave_label, "Octave: 4");
    lv_obj_align(octave_label, LV_ALIGN_TOP_LEFT, 0, 10);

    // Create a button matrix for the octave control
    static const char *octave_map[] = {
        "Up", "\n",
        "Down", ""
    };

    lv_obj_t *octave_btnm = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(octave_btnm, octave_map);
    lv_obj_set_size(octave_btnm, 150, 60);
    lv_obj_align(octave_btnm, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(octave_btnm, octave_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Create a button matrix for the notes
    static const char *note_map[] = {
        "0", "1", "2", "3", "\n",
        "4", "5", "6", "7", "\n",
        "8", "9", "10", "11", ""
    };

    lv_obj_t *note_btnm = lv_btnmatrix_create(lv_scr_act());
    lv_btnmatrix_set_map(note_btnm, note_map);
    lv_obj_set_size(note_btnm, 320, 150);
    lv_obj_align(note_btnm, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(note_btnm, note_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Create the tone queue
    tone_queue = xQueueCreate(10, sizeof(tone_t));
    assert(tone_queue != NULL);

    // Create the tone task
    xTaskCreate(tone_task, "tone_task", 2048, NULL, 5, NULL);

    bsp_display_backlight_on();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
