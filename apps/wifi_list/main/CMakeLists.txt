idf_component_register(SRCS "wifi_list.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_event nvs_flash esp_netif)
