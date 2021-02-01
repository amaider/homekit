# TEMPLATE

## getter, setter:
```c
homekit_value_t brightness_on_get() {return HOMEKIT_INT(led_brightness);}

void brightness_on_set(homekit_value_t value) {
    if (value.format != homekit_format_int) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }
    led_brightness = value.int_value;
    led_string_set();
}
```

## callback:
```c
void brightness_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {

}
```

## notify:
```c
homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "name");

homekit_characteristic_notify(&name, HOMEKIT_STRING("example"));
```

## homekit service
```c
HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
    HOMEKIT_CHARACTERISTIC(
        &name,
        BRIGHTNESS,
        100,
        .min_value = (float[]) {0},
        .max_value = (float[]) {100},
        .step_value = (float[]) {1},
        .getter=brightness_on_get,
        .setter=brightness_on_set,
        .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(brightness_callback)
    ),
    NULL
}),
```

## initialize:
```c
// create wifi network
void on_wifi_ready() {
    homekit_server_init(&config);
}
wifi_config_init("amaider - LED Tiles ", NULL, on_wifi_ready);  //replace NULL with passwd

// with wifi.h
wifi_init();
homekit_server_init(&config);
```

# Makefile
```c
extras/i2c \

extras/bmp280 \

extras/ssd1306 \
extras/fonts \

extras/i2s_dma \
extras/ws2812_i2s \

$(abspath ../../esp-homekit-demo/components/esp8266-open-rtos/wifi_config) \    // for AP Wifi
$(abspath ../../esp-homekit-demo/components/common/button) \    // for occupancy detector
```