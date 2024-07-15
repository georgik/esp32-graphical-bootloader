# ESP32 Graphical Bootloader

3rd stage graphical bootloader which let's you pick an applications whic are stored in OTA partitions.

The default configuration for ESP32-S3-BOX.

For ESP32-S3-BOX-3 or M5Stack-CoreS3 - uncomment BSP in `idf_component.yml`


## Quick start

```shell
idf.py build flash
pushd apps/tic_tac_toe
idf.py build
esptool.py --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash 0x220000 build/tic_tac_toe.bin
popd
pushd apps/wifi_list
esptool.py --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash 0x4E0000 build/wifi_list.bin
popd
pushd apps/calculator
esptool.py --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash 0x7A0000 build/calculator.bin
popd
pushd apps/synth_piano
esptool.py --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash 0xA60000 build/synth_piano.bin
popd
pushd apps/game_of_life
esptool.py --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash 0xD20000 build/game_of_life.bin
popd
```

## Build

Initial build and flash of the application and partition table.

```shell
idf.py build flash monitor
```

After the initial flash, it's possible to use following command, just to update the factory application:

```shell
idf.py app-flash monitor
```

## Flashing apps

Applications are stored in ota_0 - ota_4.

Build application (e.g. hello_world):
```shell
idf.py build
```

Flash applications to ota_0 using [espflash](https://github.com/esp-rs/espflash/blob/main/espflash/README.md#installation):
```
espflash write-bin 0xD20000 .\build\app.bin
```

Alternatively using `esptool.py`:
```
esptool.py --chip esp32s3  --baud 921600 --before default_reset --after hard_reset write_flash 0xD20000 build/hello_world.bin
```

Change offset for other apps:
- ota_0 - 0x220000
- ota_1 - 0x4E0000
- ota_2 - 0x7A0000
- ota_3 - 0xA60000
- ota_4 - 0xD20000

## Updating apps to fallback to bootloader

The bootloader is using OTA mechanism. It's necessary to add following code to the application
in order to reboot to bootloader.

Put the following code to main, before starting the rest of the application:
```c
#include "esp_ota_ops.h"

const esp_partition_t* factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
if (factory_partition != NULL) {
    esp_ota_set_boot_partition(factory_partition);
}
```

Here's more elaborate version which can be put somwhere into application, like reaction on back button:

```c
#include "esp_ota_ops.h"

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
printf("Restarting now.\n");
esp_restart();
```

If the project is using explicit list of components, you need to add `app_update` into `main/CMakeLists.txt`, so it looks like this:
```
idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES app_update
)
```
