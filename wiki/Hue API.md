# Hue API
```c
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
 * @return TRUE if response is found in buf
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
```
## example
```c
void example() {
    // 1.)
    // SET example
    snprintf(hue_request, 260, "PUT" HUE_URL1 "/lights/%i/state " HUE_URL2 "{\"on\":true,\"bri\":%i}\r\n", (int)context, (uint8_t)(value.int_value*2.55));
    // GET example
    snprintf(hue_request, 260, "GET" HUE_URL1 "/lights/4 " HUE_URL2);


    // 2.)
    //send HTTP request
    http_send_request(hue_request, sizeof(hue_request), "\"error\"");
    //sends repeatedly HTTP requests until response has no error
    while(http_send_request(hue_request, sizeof(hue_request), "\"error\"")) {
        DEBUG("http_send_request failed");
        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
```