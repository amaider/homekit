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

#define debug(fmt, ...) printf("%s" fmt "\n", "bme280_pir.c: ", ## __VA_ARGS__);

/*
 * BME280
 */
#include "i2c/i2c.h"
#include "bmp280/bmp280.h"
const uint8_t i2c_bus = 0;
const uint8_t scl_pin = 5;
const uint8_t sda_pin = 4;

float temperature_value, pressure_value, humidity_value;

homekit_characteristic_t temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t humidity    = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

void bmp280_sensor_task(void *pvParameters) {
    bmp280_params_t  params;

    bmp280_init_default_params(&params);

    bmp280_t bmp280_dev;
    bmp280_dev.i2c_dev.bus = i2c_bus;
    bmp280_dev.i2c_dev.addr = BMP280_I2C_ADDRESS_0;

    while (1) {
        while (!bmp280_init(&bmp280_dev, &params)) {
            debug("BMP280 initialization failed\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        while(1) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (!bmp280_read_float(&bmp280_dev, &temperature_value, &pressure_value, &humidity_value)) {
                debug("Temperature/pressure reading failed\n");
                break;
            }
            //printf("Humidity: %.2f Pa, Temperature: %.2f C\n", humidity_value, temperature_value);
            temperature.value.float_value = temperature_value;
            humidity.value.float_value = humidity_value;
            homekit_characteristic_notify(&temperature, HOMEKIT_FLOAT(temperature_value));
            homekit_characteristic_notify(&humidity, HOMEKIT_FLOAT(humidity_value));
            vTaskDelay(3000 / portTICK_PERIOD_MS);
        }
    }
}

/*
 * PIR Sensor
 */
#include "../../esp-homekit-demo/components/common/button/toggle.h"
#define SENSOR_PIN 14

homekit_characteristic_t occupancy_detected = HOMEKIT_CHARACTERISTIC_(OCCUPANCY_DETECTED, 0);

void sensor_callback(bool high, void *context) {
    occupancy_detected.value = HOMEKIT_UINT8(high ? 1 : 0);
    homekit_characteristic_notify(&occupancy_detected, occupancy_detected.value);
}

/*
 * homekit accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Temp, Hum, PIR Sensor"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "BME280, HC-SR501"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Temperature Sensor"),
            &temperature,
            NULL
        }),
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
            &humidity,
            NULL
        }),
        HOMEKIT_SERVICE(OCCUPANCY_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "PIR Sensor"),
            &occupancy_detected,
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Switch"),
            HOMEKIT_CHARACTERISTIC(ON, false),
            NULL
        }),
        NULL
    }),
    NULL
};

/*
 * inits
 */
sensor_init() {
    if (toggle_create(SENSOR_PIN, sensor_callback, NULL)) {
        debug("Failed to initialize sensor\n");
    }
    i2c_init(i2c_bus, scl_pin, sda_pin, I2C_FREQ_400K);
    xTaskCreate(bmp280_sensor_task, "Temperatore Sensor", 256, NULL, 2, NULL);
}

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
    sensor_init();
    homekit_server_init(&config);
}
