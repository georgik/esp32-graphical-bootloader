idf_component_register(SRCS "wifi_list.c"
                    INCLUDE_DIRS "."
                    REQUIRES app_update esp_wifi nvs_flash)
