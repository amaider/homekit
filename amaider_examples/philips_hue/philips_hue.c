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

/* Serial print use: debug("%s: <your print here>", __func__); */
#if 1
#define debug(fmt, ...) printf("%s" fmt "\n", "philips_hue.c: ", ## __VA_ARGS__);
#define CHECKBOX    "\t\xe2\x98\x90"
#define SUCCESSFUL  "\t\xe2\x9c\x93"
#define FAILED      "\t\xe2\x9c\x95"
#else
#define debug(fmt, ...)
#endif

/*
 * HUE API
 */
#include "lwip/api.h"

#define HOST        "Host: " HUE_BRIDGE_IP "\r\n"
#define CONN        "Connection: keep-alive\r\n"
#define TYPE        "Content-Type: text/plain;charset=UTF-8\r\n"
#define CON_LEN     "Content-Length: 70\r\n"

void http_sent_request(char *request, uint16_t length) {
    struct netconn *conn;
    err_t err;
    
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;

    /* Create a new TCP connection handle */
    conn = netconn_new(NETCONN_TCP);
    if (conn != NULL) {
        debug("netconn_new()\t" SUCCESSFUL);
        /* define the server address and port */
        /* 192.168.0.6  = 0xC0 . 0xA8 . 0x00 . 0x06 ==> 0x0600A8C0 */
        const ip_addr_t ipaddr = { (u32_t)0x0600A8C0 };
        const u16_t port = 80;
        
        /* connect to ip address and port */
        err = netconn_connect(conn, &ipaddr, port);
        if(err != ERR_OK) {
            debug("netconn_connect()\t" FAILED "\terr = %d", err);
        } else {
            debug("netconn_connect()\t" SUCCESSFUL);
            /* write the HTTP request to the server */
            err = netconn_write(conn, (const unsigned char*)request, (size_t)length, NETCONN_NOCOPY);
            if(err != ERR_OK) {debug("netconn_write()\t" FAILED "\terr = %d", err);} else {debug("netconn_write()\t" SUCCESSFUL);}

            /* Read the data from the port, blocking if nothing yet there yet. */
            err = netconn_recv(conn, &inbuf);
            if(err != ERR_OK) {
                debug("netconn_recv()\t" FAILED "\terr = %d", err);
            } else {
                debug("netconn_recv()\t" SUCCESSFUL);

                /* recv data in buffer */
                err = netbuf_data(inbuf, (void**) &buf, &buflen);
                if(err != ERR_OK) {
                    debug("netbuf_data()\t" FAILED "\terr = %d", err);
                } else {
                    printf("\nbuf:");
                    printf(buf);
                    printf("\nbuflength: %d\n", buflen);

                    /* search for success*/
                    if(lwip_strnstr(buf, "\"success\"", buflen)) {debug("hue has success\t" SUCCESSFUL);} else {debug("hue has success\t" FAILED);}
                    if(lwip_strnstr(buf, "\"error\"", buflen)) {debug("hue has error\t" SUCCESSFUL);} else {debug("hue has error\t" FAILED);}

                    /* release the buffer */
                    debug("netbuf_delete()\t" SUCCESSFUL FAILED);
                    netbuf_delete(inbuf);                
                }
            }

            /* close the connection */
            err = netconn_close(conn);
            if(err != ERR_OK) {debug("netconn_close()\t" FAILED "\terr = %d", err);} else {debug("netconn_close()\t" SUCCESSFUL);}
        }

        /* delete TCP connection */
        err = netconn_delete(conn);
        if(err != ERR_OK) {debug("netconn_delete()\t" FAILED);} else {debug("netconn_delete()\t" SUCCESSFUL);}
    } else {
        debug("netconn_new() returned NULL");
    }
}

void hue_url(char *category, uint8_t num, char *request) {
    char hue_request[260];
    snprintf(hue_request, 260, "PUT /api/" HUE_USER_API "/%s/%i/state HTTP/1.1\r\n" HOST CONN TYPE CON_LEN "\r\n %s\r\n", category, num, request);
    printf("\n");
    printf(hue_request);
    printf("\n");
    http_sent_request(hue_request, sizeof(hue_request));
    vTaskDelay(100 / portTICK_PERIOD_MS);
    debug("%s: ", __func__);
}

void hue_set1(char *category, uint8_t num, char *arg1, uint16_t val1) {
    char hue_body[70];
    if(strcmp(arg1, "on") == 0) {
        snprintf(hue_body, 70, "{\"%s\":%s}", arg1, val1 ? "true" : "false");
    } else {
        snprintf(hue_body, 70, "{\"%s\":%i}", arg1, val1);
    }
    hue_url(category, num, hue_body);
}
void hue_set2(char *category, uint8_t num, char *arg1, uint16_t val1, char *arg2, uint16_t val2) {
    char hue_body[70];
    if(strcmp(arg1, "on") == 0) {
        snprintf(hue_body, 70, "{\"%s\":%s,\"%s\":%i}", arg1, val1 ? "true" : "false", arg2, val2);
    } else {
        snprintf(hue_body, 70, "{\"%s\":%i,\"%s\":%i}", arg1, val1, arg2, val2);
    }
    hue_url(category, num, hue_body);
}
void hue_set3(char *category, uint8_t num, char *arg1, uint16_t val1, char *arg2, uint16_t val2, char *arg3, uint16_t val3) {
    char hue_body[70];
    if(strcmp(arg1, "on") == 0) {
        snprintf(hue_body, 70, "{\"%s\":%s,\"%s\":%i,\"%s\":%i}", arg1, val1 ? "true" : "false", arg2, val2, arg3, val3);
    } else {
        snprintf(hue_body, 70, "{\"%s\":%i,\"%s\":%i,\"%s\":%i}", arg1, val1, arg2, val2, arg3, val3);

    }
    hue_url(category, num, hue_body);
}
void hue_set4(char *category, uint8_t num, char *arg1, uint16_t val1, char *arg2, uint16_t val2, char *arg3, uint16_t val3, char *arg4, uint16_t val4) {
    char hue_body[70];
    if(strcmp(arg1, "on") == 0) {
        snprintf(hue_body, 70, "{\"%s\":%s,\"%s\":%i,\"%s\":%i,\"%s\":%i}", arg1, val1 ? "true" : "false", arg2, val2, arg3, val3, arg4, val4);
    } else {
        snprintf(hue_body, 70, "{\"%s\":%i,\"%s\":%i,\"%s\":%i,\"%s\":%i}", arg1, val1, arg2, val2, arg3, val3, arg4, val4);
    }
    hue_url(category, num, hue_body);
}
void hue_set5(char *category, uint8_t num, char *arg1, uint16_t val1, char *arg2, uint16_t val2, char *arg3, uint16_t val3, char *arg4, uint16_t val4, char *arg5, uint16_t val5) {
    char hue_body[70];
    if(strcmp(arg1, "on") == 0) {
        snprintf(hue_body, 70, "{\"%s\":%s,\"%s\":%i,\"%s\":%i,\"%s\":%i,\"%s\":%i}", arg1, val1 ? "true" : "false", arg2, val2, arg3, val3, arg4, val4, arg5, val5);
    } else {
        snprintf(hue_body, 70, "{\"%s\":%i,\"%s\":%i,\"%s\":%i,\"%s\":%i,\"%s\":%i}", arg1, val1, arg2, val2, arg3, val3, arg4, val4, arg5, val5);
    }
    hue_url(category, num, hue_body);
}

/*
 * Lightbulb
 */
void light_on_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    hue_set1("lights", 5, "on", value.int_value);
    debug("%s: ", __func__);
}
void light_bri_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    hue_set1("lights", 5, "bri", (uint8_t)(value.int_value*2.55));
    debug("%s: ", __func__);
}

/*
 * homekit accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "philips_hue"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "001"),
            HOMEKIT_CHARACTERISTIC(MODEL, "NodeMCU 1.0"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "14.02.2021"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(light_on_callback)),
            HOMEKIT_CHARACTERISTIC(BRIGHTNESS, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(light_bri_callback)),
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
