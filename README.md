# ESP32 Graphical Bootloader

3rd stage graphical bootloader which let's you pick an applications whic are stored in OTA partitions.

The default configuration for ESP32-S3-BOX-3.

For M5Stack-CoreS3 - uncomment BSP in `idf_component.yml`


## Build

Initial build and flash of the application and partition table.

```
idf.py build flash monitor
```

After the initial flash, it's possible to use following command, just to update the factory application:

```
idf.py app-flash monitor
```

## Flashing apps

Applications are stored in ota_0 - ota_4.

Build application (e.g. hello_world):
```
idf.py build
```

Flash applications to ota_0:
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
in order to reboot to bootloader:

```
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