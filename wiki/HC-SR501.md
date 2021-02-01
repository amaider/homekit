# TEMPLATE

## #include
```c
#include "../../esp-homekit-demo/components/common/button/toggle.h"
```

## variables:
```c
#define SENSOR_PIN 0

homekit_characteristic_t occupancy_detected = HOMEKIT_CHARACTERISTIC_(OCCUPANCY_DETECTED, 0);
```

## main task:
```c
void sensor_callback(bool high, void *context) {
    occupancy_detected.value = HOMEKIT_UINT8(high ? 1 : 0);
    homekit_characteristic_notify(&occupancy_detected, occupancy_detected.value);
}
```

## homekit services:
```c
HOMEKIT_SERVICE(OCCUPANCY_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
    HOMEKIT_CHARACTERISTIC(NAME, "PIR Sensor"),
    &occupancy_detected,
    NULL
}),
```

## initialize:
```c
/* 0 way */
toggle_create(SENSOR_PIN, sensor_callback, NULL);

/* 1st way */
if (toggle_create(SENSOR_PIN, sensor_callback, NULL)) {
    debug("Failed to initialize sensor\n");
}

/* 2nd way */
void pir_init() {
    if (toggle_create(SENSOR_PIN, sensor_callback, NULL)) {
        debug("Failed to initialize sensor\n");
    }
}


pir_init();
```

# Makefile
```c
$(abspath ../../esp-homekit-demo/components/common/button) \
```