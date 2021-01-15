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

#define debug(fmt, ...) printf("%s" fmt "\n", "###Security_System_PIR.c: ", ## __VA_ARGS__);

/*
 * Security System
 */
homekit_characteristic_t current_state = HOMEKIT_CHARACTERISTIC_(SECURITY_SYSTEM_CURRENT_STATE, 0);
void security_target_state_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    homekit_characteristic_notify(&current_state, HOMEKIT_UINT8(value.int_value));
    debug("%s: target_state=%i", __func__, value.int_value);
}

/*
 * homekit accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "PIR Alarm"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "HC-SR501"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(SECURITY_SYSTEM, .primary = true, .characteristics = (homekit_characteristic_t*[]) {
            &current_state,
            HOMEKIT_CHARACTERISTIC(SECURITY_SYSTEM_TARGET_STATE, 3, .min_value = (float[]) {2}, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(security_target_state_callback)),
            HOMEKIT_CHARACTERISTIC(NAME, "PIR Alarm"),
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
