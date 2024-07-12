idf_component_register(SRCS
    "bootloader_ui.c"
    "graphical_bootloader_main.c"

    INCLUDE_DIRS
        ".")

lvgl_port_create_c_image("../resources/images/icon_tic_tac_toe.png" "images/gen/" "ARGB8888" "NONE")
lvgl_port_create_c_image("../resources/images/icon_wifi_list.png" "images/gen/" "ARGB8888" "NONE")
lvgl_port_add_images(${COMPONENT_LIB} "images/gen/")
