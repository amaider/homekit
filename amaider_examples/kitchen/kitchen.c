/**
 * HC-SR501, BME280, Hue API
 * #define HUE LEDs in Light setters
 * #define PINS
 * #define HUE_BRIDGE_IP and HUE_USER_API
 */
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

#include "i2c/i2c.h"
#include "bmp280/bmp280.h"
#include "../../esp-homekit-demo/components/common/button/toggle.h"

/** Serial print use: DEBUG("%s: your_print_here", __FILENAME__); */
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DEBUG(message, ...) printf("%s: " message "\n", __func__, ##__VA_ARGS__)

/** BME280 */
#define I2C_BUS 0
#define SCL_PIN 5   //RX    ESP-01: 3   ESP12E: 5
#define SDA_PIN 4   //TX    ESP-01: 1   ESP12E: 4
float temperature_value, pressure_value, humidity_value;
homekit_characteristic_t temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t humidity = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);
homekit_characteristic_t temperature_status_tampered = HOMEKIT_CHARACTERISTIC_(STATUS_TAMPERED, 0);
homekit_characteristic_t humidity_status_tampered = HOMEKIT_CHARACTERISTIC_(STATUS_TAMPERED, 0);

void bmp280_sensor_task(void *pvParameters) {
    bmp280_params_t  params;

    bmp280_init_default_params(&params);

    bmp280_t bmp280_dev;
    bmp280_dev.i2c_dev.bus = I2C_BUS;
    bmp280_dev.i2c_dev.addr = BMP280_I2C_ADDRESS_0;

    while (1) {
        while (!bmp280_init(&bmp280_dev, &params)) {
            DEBUG("BMP280 initialization failed");
            homekit_characteristic_notify(&temperature_status_tampered, HOMEKIT_UINT8(1));
            homekit_characteristic_notify(&humidity_status_tampered, HOMEKIT_UINT8(1));
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        while(1) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            if (!bmp280_read_float(&bmp280_dev, &temperature_value, &pressure_value, &humidity_value)) {
                DEBUG("Temperature/pressure reading failed");
                homekit_characteristic_notify(&temperature_status_tampered, HOMEKIT_UINT8(1));
                homekit_characteristic_notify(&humidity_status_tampered, HOMEKIT_UINT8(1));
                break;
            }
            //DEBUG("Humidity: %.2f Pa, Temperature: %.2f C\n", humidity_value, temperature_value);
            temperature.value.float_value = temperature_value;
            humidity.value.float_value = humidity_value;
            homekit_characteristic_notify(&temperature, HOMEKIT_FLOAT(temperature_value));
            homekit_characteristic_notify(&humidity, HOMEKIT_FLOAT(humidity_value));
            homekit_characteristic_notify(&temperature_status_tampered, HOMEKIT_UINT8(0));
            homekit_characteristic_notify(&humidity_status_tampered, HOMEKIT_UINT8(0));
            vTaskDelay(4500 / portTICK_PERIOD_MS);
            DEBUG("BMP read data");
        }
    }
}

/** 
 * Hue API
 * example:
 * snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/3/state " HUE_URL2 "{\"on\":true,\"bri\":15,\"ct\":153}\r\n");
 * http_send_request(hue_request, sizeof(hue_request), "\"error\"");
 */
#include "lwip/api.h"
#define HUE_URL1 " /api/" HUE_USER_API
#define HUE_URL2 "HTTP/1.1\r\nHost: " HUE_BRIDGE_IP "\r\nConnection: keep-alive\r\nContent-Type: text/plain;charset=UTF-8\r\nContent-Length: 70\r\n\r\n"
char hue_request[260];
/** 
 * @brief sends the HTTP request and scans answer for given string
 * @param request is the HTTP URL/body
 * @param length of request
 * @param response is the string for which will be searched in the HTTP response
 * @return TRUE if response is found
 */
bool http_send_request(char *request, uint16_t length, char *response) {
    bool success = false;
    struct netconn *conn;
    err_t err;
    
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;

    /* Create a new TCP connection handle */
    conn = netconn_new(NETCONN_TCP);
    if (conn != NULL) {
        /* define the server address and port */
        /* 192.168.0.6  = 0xC0 . 0xA8 . 0x00 . 0x06 ==> 0x0600A8C0 */
        const ip_addr_t ipaddr = { (u32_t)0x0600A8C0 };
        const u16_t port = 80;
        
        /* connect to ip address and port */
        err = netconn_connect(conn, &ipaddr, port);
        if(err != ERR_OK) {
            DEBUG("netconn_connect() err = %d", err);
        } else {
            /* write the HTTP request to the server */
            err = netconn_write(conn, (const unsigned char*)request, (size_t)length, NETCONN_NOCOPY);
            if(err != ERR_OK) {
                DEBUG("netconn_write() err = %d", err);
            } else {
                /* Read the data from the port, blocking if nothing yet there yet. */
                err = netconn_recv(conn, &inbuf);
                if(err != ERR_OK) {
                    DEBUG("netconn_recv() err = %d", err);
                    // break;
                } else {
                    /* recv data in buffer */
                    err = netbuf_data(inbuf, (void**) &buf, &buflen);
                    if(err != ERR_OK) {
                        DEBUG("netbuf_data() err = %d", err);
                        // break;
                    } else {
                        /* printf request, buf, and buflength for DEBUGGING */
                        // DEBUG("request: %s", request);
                        // DEBUG("buf: %s", buf);
                        // DEBUG("buflen: %i", buflen);

                        /* process HUE API command response (buf) by searching for char *response */
                        if(lwip_strnstr(buf, response, buflen)) {
                            success = true;
                            // DEBUG("buf includes %s", response);
                        } else {
                            success = false;
                            // DEBUG("buf does NOT include %s", response);
                        }
                        /* release the buffer */
                        netbuf_delete(inbuf);                
                    }
                }
            }
            /* close the connection */
            err = netconn_close(conn);
            if(err != ERR_OK) {DEBUG("netconn_close() err = %d", err);}
        }
        /* delete TCP connection */
        err = netconn_delete(conn);
        if(err != ERR_OK) {DEBUG("netconn_delete() err = &d");}
    } else {
        DEBUG("netconn_new() returned NULL");
    }
    return success;
}

/** xTimer */
#include <timers.h>
TimerHandle_t timer_handle = NULL;

/** the two main functions */
void hue_on() {
    snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/3/state " HUE_URL2 "{\"on\":true,\"bri\":255,\"ct\":153}\r\n");
    while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
        DEBUG("http_send_request failed");
        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/4/state " HUE_URL2 "{\"on\":true,\"bri\":255}\r\n");
    while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
        DEBUG("http_send_request failed or error not found");
        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
void hue_off() {
    snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/3/state " HUE_URL2 "{\"on\":false}\r\n");
    while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
        DEBUG("http_send_request failed");
        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/4/state " HUE_URL2 "{\"on\":false}\r\n");
    while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
        DEBUG("http_send_request failed");
        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if(xTimerDelete(timer_handle, 0) != pdPASS) {DEBUG("xTimerDelete() failed");} else {DEBUG("xTimerDelete()");}
}

/** PIR HC-SR501 */
#define SENSOR_PIN 14   // ESP-01: 2  ESP12E: 14
homekit_characteristic_t motion_detected = HOMEKIT_CHARACTERISTIC_(MOTION_DETECTED, 0);
void sensor_callback(bool high, void *context) {
    DEBUG("value = %i", high);
    motion_detected.value = HOMEKIT_BOOL(high);
    homekit_characteristic_notify(&motion_detected, motion_detected.value);

    if(high){
        /* check if current lights are NOT on */
        snprintf(hue_request, 260, "GET" HUE_URL1 "/lights/3 " HUE_URL2);
        bool status_rgb = http_send_request(hue_request, sizeof(hue_request), "\"on\":false");
        snprintf(hue_request, 260, "GET" HUE_URL1 "/lights/4 " HUE_URL2);
        bool status_w = http_send_request(hue_request, sizeof(hue_request), "\"on\":false");
        DEBUG("status_rgb = %d, status_w = %d", status_rgb, status_w);
        if(status_rgb && status_w) {
            hue_on();
            timer_handle = xTimerCreate("set_off_timer", 60 * (1000 / portTICK_PERIOD_MS), pdFALSE, 0, hue_off);
            if(xTimerReset(timer_handle, 0) != pdPASS) {DEBUG("xTimerReset() failed");} else {DEBUG("xTimerReset()");}
        }
    }
}

/** homekit accessories */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_security_system, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Küche"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "BME280, HC-SR501, Philips Hue"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP12F"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "26.05.2021"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Küche BME280"),
            &temperature,
            &temperature_status_tampered,
            NULL
        }),
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Küche BME280"),
            &humidity,
            &humidity_status_tampered,
            NULL
        }),
        HOMEKIT_SERVICE(MOTION_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Küche HC-SR501"),
            &motion_detected,
            NULL
        }),
        NULL
    }),
    NULL
};

/* inits */
/**
 * @brief after ESP12F crash (because of xTimerReset), this task makes sure the lights are off because the timer cant
 */
void wifi_check(void *pvParameters) {
    while(1) {
        while(sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
            DEBUG("WiFi not connected");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        DEBUG("WiFi connected");
        snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/3/state " HUE_URL2 "{\"on\":false}\r\n");
        while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
            DEBUG("http_send_request failed");
            // vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/4/state " HUE_URL2 "{\"on\":false}\r\n");
        while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
            DEBUG("http_send_request failed");
            // vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        DEBUG("Hue set off");
        vTaskDelete(NULL);
    }
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
    homekit_server_init(&config);
    
    i2c_init(I2C_BUS, SCL_PIN, SDA_PIN, I2C_FREQ_400K);
    gpio_enable(SENSOR_PIN, GPIO_INPUT);
    xTaskCreate(bmp280_sensor_task, "Temperatore Sensor", 256, NULL, 2, NULL);
    if (toggle_create(SENSOR_PIN, sensor_callback, NULL)) {
        DEBUG("Failed to initialize sensor");
    }
    xTaskCreate(wifi_check, "wifi_check", 512, NULL, 2, NULL);
}
