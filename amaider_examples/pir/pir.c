#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include "../../esp-homekit-demo/wifi.h"

#include "../../esp-homekit-demo/components/common/button/toggle.h"

#define debug(fmt, ...) printf("%s" fmt "\n", "pir.c: ", ## __VA_ARGS__);

/*
 * PIR Sensor
 */
#define SENSOR_PIN 0

homekit_characteristic_t occupancy_detected = HOMEKIT_CHARACTERISTIC_(OCCUPANCY_DETECTED, 0);

void sensor_callback(bool high, void *context) {
    occupancy_detected.value = HOMEKIT_UINT8(high ? 1 : 0);
    homekit_characteristic_notify(&occupancy_detected, occupancy_detected.value);
}

void pir_init() {
    if (toggle_create(SENSOR_PIN, sensor_callback, NULL)) {
        debug("Failed to initialize sensor\n");
    }
}

/*
 * homekit accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "PIR Sensor"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "HC-SR501"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(OCCUPANCY_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "PIR Sensor"),
            &occupancy_detected,
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
    pir_init();
    homekit_server_init(&config);
}
