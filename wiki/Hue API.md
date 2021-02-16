# Hue API

## #include
```c
#include "lwip/api.h"
```

## #define:
```c
#define HOST        "Host: " HUE_BRIDGE_IP "\r\n"
#define CONN        "Connection: keep-alive\r\n"
#define TYPE        "Content-Type: text/plain;charset=UTF-8\r\n"
#define CON_LEN     "Content-Length: 70\r\n"
```

## main task:
```c
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
```

## homekit services:
```c
N/A
```

## initialize:
```c
N/A
```

# Makefile
```c
N/A
```