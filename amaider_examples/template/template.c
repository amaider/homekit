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

#define debug(fmt, ...) printf("%s" fmt "\n", "template.c: ", ## __VA_ARGS__);

/*
 * null_identify
 */
#define LED_GPIO 2
void null_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            gpio_write(LED_GPIO, 1);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            gpio_write(LED_GPIO, 0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
    gpio_write(LED_GPIO, 0);
    vTaskDelete(NULL);
}

void template_identify(homekit_value_t _value) {
    debug("ACCESSORY_INFORMATION identify\n");
    xTaskCreate(null_identify_task, "LED identify", 128, NULL, 2, NULL);
}

/*
 * Switch
 */
homekit_characteristic_t characteristic;

void switch_on_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    homekit_characteristic_notify(&characteristic, HOMEKIT_INT(value.int_value));
    debug("%s: ", __func__);
}

homekit_characteristic_t characteristic = HOMEKIT_CHARACTERISTIC_(BRIGHTNESS, 100, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback));

/*
 * homekit accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "template"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "NodeMCU 1.0"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, template_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)),
            &characteristic,
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
