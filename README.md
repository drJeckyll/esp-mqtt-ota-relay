# THIS CODE IS OBSOLETE AND WILL NOT BE UPDATED ANYMORE
Please use new repo here: https://git.jeckyll.net/published/personal/esp8266/esp-mqtt-ota-relay


# esp-mqtt-ota-relay

This drive my 4 relays board. It is 12V relay board, but this can be used on any relay board. You can drive up to 4 relays.

* Relay 1 is connected to GPIO4
* Relay 2 is connected to GPIO14
* Relay 3 is connected to GPIO12
* Relay 4 is connected to GPIO13

Settings for WIFI, MQTT and OTA are in Makefile, or you can set them via env:
```
DEVICE=test WIFI_SSID=Test make
```

You must set DEVICE since it is used both OTA and MQTT.

Each relay is controled via separate MQTT TOPIC:
```
MQTT_PREFIX"1" - for relay 1
MQTT_PREFIX"2" - for relay 2
MQTT_PREFIX"3" - for relay 3
MQTT_PREFIX"4" - for relay 4
```

Settings are saved after each change. When device is rebooted settings are restored. After each change and after restart you will recive new settings via MQTT_PREFIX"settings/reply" topic in JSON format. To get settings just send something to MQTT_PREFIX"settings"

To reboot ESP8266 just send something to MQTT_PREFIX"restart".

To perform OTA update, first compile rom0.bin and rom1.bin. Put them on web server which can be accessed by http://OTA_HOST:OTA_PORT/OTA_PATH. For example:

```
OTA_HOST="192.168.1.1"
OTA_PORT=80
OTA_PATH="/firmware/"
```

Then just send someting to MQTT_PREFIX"update". After 10-15 seconds update will be done.

There is no version control of bin files. Update is performed every time no matter if it is old ot new bin file.

If you use BOOT_CONFIG_CHKSUM and BOOT_IROM_CHKSUM (and you should - see warning bellow) and update failed device will return with old bin. You can check which bin is loaded by check settings and see rom:0 (for example). After update succes it will be rom:1. Else it will be rom:0 again, so you must perform update again.

You need:

* rboot boot loader: https://github.com/raburton/rboot
* esptool2: https://github.com/raburton/esptool2

WARNING: rboot must be compiled with BOOT_CONFIG_CHKSUM and BOOT_IROM_CHKSUM in rboot.h or it will not boot.

You can remove -iromchksum from FW_USER_ARGS in Makefile and use default settings but OTA update will be unrealible - corrupt roms & etc.

If BOOT_CONFIG_CHKSUM and BOOT_IROM_CHKSUM are enabled then rBoot wi-iromchksumll recover from last booted rom and OTA update is much more stable.

This code was tested with esp-open-sdk (SDK 1.4.0). Flash size 1MB (8Mbit) or more.
