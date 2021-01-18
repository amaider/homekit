#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "../../esp-homekit-demo/wifi.h"

#define debug(fmt, ...) printf("%s" fmt "\n", "test.c: ", ## __VA_ARGS__);


homekit_characteristic_t characteristic;

void switch_on_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    debug("%s", __func__);
}

homekit_characteristic_t characteristic = HOMEKIT_CHARACTERISTIC_(BRIGHTNESS, 100, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback));

/*
 * homekit accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "id1"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "id1_man"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "NodeMCU 1.0"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(OCCUPANCY_SENSOR, .primary=false, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "id1_occupancy"),
            HOMEKIT_CHARACTERISTIC(OCCUPANCY_DETECTED, 0),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=false, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)),
            HOMEKIT_CHARACTERISTIC(NAME, "id1_light"),
            NULL
        }),
        NULL
    }),
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "id2"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "002"),
            HOMEKIT_CHARACTERISTIC(MODEL, "NodeMCU 1.0"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.2"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)),
            HOMEKIT_CHARACTERISTIC(NAME, "id2_switch1"),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=false, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)),
            HOMEKIT_CHARACTERISTIC(NAME, "id2_switch2"),
            NULL
        }),
        NULL
    }),
    NULL
};

/*
 * inits
 */
static void wifi_init() {
    struct sdk_station_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
    };

    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&wifi_config);
    sdk_wifi_station_connect();
}

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void user_init(void) {
    wifi_init();
    homekit_server_init(&config);
}
