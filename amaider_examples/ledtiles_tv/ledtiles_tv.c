//LED Pin = GPIO3 (RX pin)

//remove unnecessary variabes (led_on,...); printf, makefile, modes/colorwheels
// testing: (WS2812_show) _mode_delay vs. vTaskDelay in _color_modes
// testing: add/revisit _mode in every loop, and test effect
//min_value brightness 8, sonst low voltage und nicht alle farben leuchten
//rainbow under 30 brightness getting choppy? -> min_value 30

//esp-wifi-config: sonoff_basic_pmw/main.c
//extras/mdnsresponder
//manipulate wifi_config into own website, or combine both

//website: reset filter indication, on 1 open website, if done return 0
//website as Lock control point(user input?), lock physical controls, lock switch, Hold position,
//watch safari inspect elements/script as wifi-config on join button press

//manual mode change with touch-pcb, check with: lock_open/on_lock_open(on_active)=true -> i++ -> setMode(i)

//in mode fucntions uint16_t to normal int, bit reduction->saving space?

// https://www.instructables.com/How-to-Make-Proper-Rainbow-and-Random-Colors-With-/


#include <stdio.h>
#include <stdlib.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <math.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
//#include <wifi_config.h>
#include "../../esp-homekit-demo/wifi.h"   //remove, if no ssid/passwd

#include "ledtiles_WS2812FX.h"
#include "ledtiles_WS2812FX.c"

#define debug(fmt, ...) printf("%s" fmt "\n", "amaider_ledtiles_tv.c: ", ## __VA_ARGS__);

/*
 * HOMEKIT SSID/PASSWORD Set/Reset
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

homekit_characteristic_t name_source1 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "FX Modes");
homekit_characteristic_t tv_configured_name = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "tvteleeeee");

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "user_init");

/*
 * TV callbacks, Mode actions, case laws
 */
//tv_shutoff function with 2nd brighntess variable and current_brightness = 0 for editMode
void tv_active_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    value.bool_value ? WS2812FX_forceBrightness(current_brightness) : WS2812FX_forceBrightness(0);
    //homekit_characteristic_notify(&name_source1, HOMEKIT_STRING(modename[current_mode]));
    debug("tv_active_callback: value=%i ? WS2812FX_forceBrightness(%i) : WS2812FX_forceBrightness(0)", value.bool_value, current_brightness);
}
void tv_active_identifier_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    switch (value.int_value) {
    case 1:
        editMode = false;
        WS2812FX_setMode(current_mode);
        //homekit_characteristic_notify(&name_source1, HOMEKIT_STRING(modename[current_mode]));
        homekit_characteristic_notify(&tv_configured_name, HOMEKIT_STRING("LED Tiles"));
        debug("tv_active_identifier_callback: editMode=false, WS2812FX_setMode(%i)", current_mode);
        break;
    case 2:
        editMode = true;
        WS2812FX_setMode(8);
        homekit_characteristic_notify(&tv_configured_name, HOMEKIT_STRING("LED Tiles: edit Mode"));
        debug("tv_active_identifier_callback: editMode=true, WS2812FX_setMode(8)");
        break;
    default:
        debug("tv_active_identifier_callback: Unknown active identifier: %d", value.int_value);
    }
}
void tv_power_mode_selection_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    for(uint8_t i=0; i<LED_COUNT; i++) {
        customHue[i] = rand() % 360;
        customSaturation[i] = rand() % 100;
        customBrightness[i] = rand() % 255;
    }
    debug("tv_power_mode_selection_callback: reset customArrays");
}
void tv_remote_key_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    if(!editMode) {
        switch (value.int_value) {
        case HOMEKIT_REMOTE_KEY_ARROW_UP:
            if(current_brightness<255){current_brightness += BRIGHTNESS_STEP;}
            if(current_brightness>255){current_brightness=255;}
            WS2812FX_setBrightness((uint8_t)floor(current_brightness));
            debug("tv_remote_key_callback: !editMode: arrow_up: WS2812FX_setBrightness(%i)", current_brightness);
            break;
        case HOMEKIT_REMOTE_KEY_ARROW_DOWN:
            if(current_brightness>0){current_brightness -= BRIGHTNESS_STEP;}
            if(current_brightness>255){current_brightness=0;}
            WS2812FX_setBrightness((uint8_t)floor(current_brightness));
            debug("tv_remote_key_callback: !editMode: arrow_down: WS2812FX_setBrightness(%i)", current_brightness);
            break;
        case HOMEKIT_REMOTE_KEY_ARROW_LEFT:
            current_mode = (current_mode -1) % (MODE_COUNT-1);
            if(current_mode>MODE_COUNT){current_mode=0;}
            WS2812FX_setMode(current_mode);
            homekit_characteristic_notify(&name_source1, HOMEKIT_STRING(modename[current_mode]));
            debug("tv_remote_key_callback: !editMode: arrow_left: WS2812FX_setMode(%i)", current_mode);
            break;
        case HOMEKIT_REMOTE_KEY_ARROW_RIGHT:
            current_mode = (current_mode + 1) % (MODE_COUNT-1);
            WS2812FX_setMode(current_mode);
            homekit_characteristic_notify(&name_source1, HOMEKIT_STRING(modename[current_mode]));
            debug("tv_remote_key_callback: !editMode: arrow_right: WS2812FX_setMode(%i)", current_mode);
            break;
        case HOMEKIT_REMOTE_KEY_SELECT:
            debug("tv_remote_key_callback: !editMode: select_key: ");
            break;
        case HOMEKIT_REMOTE_KEY_BACK:
            current_hue = (current_hue += HUE_STEP);
            if(current_hue>360){current_hue=0;}
            debug("tv_remote_key_callback: !editMode: back_key: current_hue=%i", current_hue);
            break;
        case HOMEKIT_REMOTE_KEY_PLAY_PAUSE:
            current_brightness = (current_brightness += BRIGHTNESS_STEP);
            if(current_brightness>255){current_brightness=0;}
            debug("tv_remote_key_callback: !editMode: play_pause_key: current_brightness=%i", current_brightness);
            /*
            is_paused = is_paused ? false : true;
            WS2812FX_setSpeed(is_paused ? 0 : current_speed);
            debug("tv_remote_key_callback: !editMode: play_pause_key: WS2812FX_setSpeed(%i)", is_paused ? 0 : current_speed);*/
            break;
        case HOMEKIT_REMOTE_KEY_INFORMATION:
            debug("tv_remote_key_callback: !editMode: info_key: ");
            uint32_t zzz = WS2812_getPixelColor(0);
            debug("zzz=%i", zzz);
            break;
        default:
            debug("Unsupported remote key code: %d", value.int_value);
        }
    }
    if(editMode) {
        switch (value.int_value) {
        case HOMEKIT_REMOTE_KEY_ARROW_UP:
            if(!is_selected) {
                if(current_brightness<255){current_brightness += BRIGHTNESS_STEP;}
                WS2812FX_setBrightness((uint8_t)floor(current_brightness));
                debug("tv_remote_key_callback: editMode: arrow_up: !is_selected: WS2812FX_setBrightness(%i)", current_brightness);
            }
            if(is_selected) {
                if(selected_brightness<255){selected_brightness += BRIGHTNESS_STEP;}
                debug("tv_remote_key_callback: editMode: arrow_up: is_selected: selected_brightness=%i", selected_brightness);
                /*selected_saturation = (selected_saturation + SATURATION_STEP) % (100+SATURATION_STEP);
                if(selected_saturation>100){selected_saturation=100;}
                debug("tv_remote_key_callback: editMode: arrow_up: is_selected: selected_saturation=%i", selected_saturation);*/
            }
            break;
        case HOMEKIT_REMOTE_KEY_ARROW_DOWN:
            if(!is_selected) {
                if(current_brightness>0){current_brightness -= BRIGHTNESS_STEP;}
                if(current_brightness>255){current_brightness = 0;}
                WS2812FX_setBrightness((uint8_t)floor(current_brightness));
                debug("tv_remote_key_callback: editMode: arrow_down: !is_selected: WS2812FX_setBrightness(%i)", current_brightness);
            }
            if(is_selected) {
                if(selected_brightness>0){selected_brightness -= BRIGHTNESS_STEP;}
                if(selected_brightness>255){selected_brightness = 0;}
                debug("tv_remote_key_callback: editMode: arrow_down: is_selected: selected_brightness=%i", selected_brightness);
                /*selected_saturation = (selected_saturation - SATURATION_STEP) % (100+SATURATION_STEP);
                if(selected_saturation>100){selected_saturation=100;}
                debug("tv_remote_key_callback: editMode: arrow_down: is_selected: selected_saturation=%i", selected_saturation);*/
            }
            break;
        case HOMEKIT_REMOTE_KEY_ARROW_LEFT:
            if(!is_selected) {
                selected_led = (selected_led - 1) % (LED_COUNT);
                if(selected_led>LED_COUNT){selected_led=(LED_COUNT-1);}
                debug("tv_remote_key_callback: editMode: arrow_left: !is_selected: selected_led=%i", selected_led);
            }
            if(is_selected) {
                selected_hue = (selected_hue - HUE_STEP) % 360;
                if(selected_hue>360){selected_hue=(360-HUE_STEP);}
                debug("tv_remote_key_callback: editMode: arrow_left: is_selected: selected_hue=%i", selected_hue);
            }
            break;
        case HOMEKIT_REMOTE_KEY_ARROW_RIGHT:
            if(!is_selected) {
                selected_led = (selected_led + 1) % (LED_COUNT);
                if(selected_led>LED_COUNT){selected_led=(LED_COUNT-1);}
                debug("tv_remote_key_callback: editMode: arrow_right: !is_selected: selected_led=%i", selected_led);
            }
            if(is_selected) {
                selected_hue = (selected_hue + HUE_STEP) % 360;
                if(selected_hue>360){selected_hue=(360-HUE_STEP);}
                debug("tv_remote_key_callback: editMode: arrow_right: is_selected: selected_hue=%i", selected_hue);
            }
            break;
        case HOMEKIT_REMOTE_KEY_SELECT:
            is_selected = is_selected ? false : true;
            if(is_selected) {debug("tv_remote_key_callback: editMode: key_select: is_selected: ");}
            if(!is_selected) {
                customHue[selected_led] = selected_hue;
                customSaturation[selected_led] = selected_saturation;
                customBrightness[selected_led] = selected_brightness;
                debug("tv_remote_key_callback: editMode: key_select: !is_selected: HUE[%i]={%i, %i, %i}", selected_led, selected_hue, selected_saturation, selected_brightness);
                //array print
                printf("customHue[%i]={", LED_COUNT);
                for(int i=0; i<LED_COUNT; i++) {
                    printf("%i,", customHue[i]);
                    if(i==LED_COUNT-1){printf("};\n");};
                }
                printf("customSaturation[%i]={", LED_COUNT);
                for(int i=0; i<LED_COUNT; i++) {
                    printf("%i,", customSaturation[i]);
                    if(i==LED_COUNT-1){printf("};\n");};
                }
                printf("customBrightness[%i]={", LED_COUNT);
                for(int i=0; i<LED_COUNT; i++) {
                    printf("%i,", customSaturation[i]);
                    if(i==LED_COUNT-1){printf("};\n");};
                }
                //array print
            }
            break;
        case HOMEKIT_REMOTE_KEY_BACK:
            debug("tv_remote_key_callback: editMode: back_key: ");
            break;
        case HOMEKIT_REMOTE_KEY_PLAY_PAUSE:
            debug("tv_remote_key_callback: editMode: play_pause_key: ");
            break;
        case HOMEKIT_REMOTE_KEY_INFORMATION:
            debug("tv_remote_key_callback: editMode: info_key: ");
            uint16_t zzz = WS2812_getPixelColor(0);
            debug("zzz=%i", zzz);
            break;
        default:
            debug("Unsupported remote key code: %d", value.int_value);
        }
    }
}
void tv_speaker_volume_selector_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg) {
    if(!editMode) {
        if(value.int_value==HOMEKIT_VOLUME_SELECTOR_INCREMENT){
            current_saturation = (current_saturation + SATURATION_STEP);
            if(current_saturation>100){current_saturation=100;}
            debug("tv_speaker_volume_selector_callback: !editMode: increment: current_saturation=%i", current_saturation);
        }
        if(value.int_value==HOMEKIT_VOLUME_SELECTOR_DECREMENT){
            current_saturation = (current_saturation - SATURATION_STEP);
            if(current_saturation>100){current_saturation=0;}
            debug("tv_speaker_volume_selector_callback: !editMode: decrement: current_saturation=%i", current_saturation);
        }
    }
    if(editMode) {
        if(value.int_value==HOMEKIT_VOLUME_SELECTOR_INCREMENT){
            selected_saturation = (selected_saturation + SATURATION_STEP);
            if(selected_saturation>100){selected_saturation=100;}
            debug("tv_speaker_volume_selector_callback: editMode: increment: selected_saturation=%i", selected_saturation);
        }
        if(value.int_value==HOMEKIT_VOLUME_SELECTOR_DECREMENT){
            selected_saturation = (selected_saturation - SATURATION_STEP);
            if(selected_saturation>100){selected_saturation=0;}
            debug("tv_speaker_volume_selector_callback: editMode: decrement: selected_saturation=%i", selected_saturation);
        }
    }
}

/*
 * INPUT SOURCES, TV SPEAKER
 */
//homekit_characteristic_t name_source1 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "FX Modes");     // used above
homekit_characteristic_t name_source2 = HOMEKIT_CHARACTERISTIC_(CONFIGURED_NAME, "edit Mode");

homekit_service_t input_source1 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics=(homekit_characteristic_t*[]){
    HOMEKIT_CHARACTERISTIC(NAME, "FX Modes"),
    HOMEKIT_CHARACTERISTIC(IDENTIFIER, 1),
    &name_source1,
    HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_HDMI),
    HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, true),
    HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN),
    NULL
});
homekit_service_t input_source2 = HOMEKIT_SERVICE_(INPUT_SOURCE, .characteristics=(homekit_characteristic_t*[]){
    HOMEKIT_CHARACTERISTIC(NAME, "Custom Colors"),
    HOMEKIT_CHARACTERISTIC(IDENTIFIER, 2),
    &name_source2,
    HOMEKIT_CHARACTERISTIC(INPUT_SOURCE_TYPE, HOMEKIT_INPUT_SOURCE_TYPE_HDMI),
    HOMEKIT_CHARACTERISTIC(IS_CONFIGURED, true),
    HOMEKIT_CHARACTERISTIC(CURRENT_VISIBILITY_STATE, HOMEKIT_CURRENT_VISIBILITY_STATE_SHOWN),
    NULL
});
homekit_service_t tv_speaker = HOMEKIT_SERVICE_(TELEVISION_SPEAKER, .characteristics=(homekit_characteristic_t*[]) {
    HOMEKIT_CHARACTERISTIC(MUTE, false),
    HOMEKIT_CHARACTERISTIC(VOLUME_SELECTOR, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(tv_speaker_volume_selector_callback)),    // 0/1 uint8_t
    HOMEKIT_CHARACTERISTIC(ACTIVE, false),
    HOMEKIT_CHARACTERISTIC(VOLUME, 7),
    HOMEKIT_CHARACTERISTIC(VOLUME_CONTROL_TYPE, HOMEKIT_VOLUME_CONTROL_TYPE_RELATIVE_WITH_CURRENT),
    HOMEKIT_CHARACTERISTIC(NAME, "tlespeaker"),
    NULL
});

/*
 * Color Picker Lightbulb
 */
/*
void picker_on_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg){
    if (value.format != homekit_format_bool) {
        printf("Invalid picker_on_callback-value format: %d\n", value.format);
        return;
    }
    picker_on = value.bool_value;
    printf("picker_on_callback: value: ");
    fputs(picker_on ? "true\n" : "false\n", stdout);
    if (picker_on == false) {
        customHue[(int)picker_brightness-1] = (int)picker_hue;
        //customSaturation[(int)picker_brightness-1] = (int)picker_saturation;

        //console array print
        printf("hue array: ");
        for(int i=0; i<25; i++){
            int a = customHue[i];
            printf("%i, ", a);
        }
        printf("\n");
        printf("sat array: ");
        for(int i=0; i<25; i++){
            int a = customSaturation[i];
            printf("%i, ", a);
        }
        printf("\n");
        //console array print
    } else {
        printf("picker_on_callback: picker is on: return\n");
        return;
    }
}
void picker_brightness_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg){
    if (value.format != homekit_format_int) {
        printf("Invalid picker_brightness_callback-value format: %d\n", value.format);
        return;
    }
    picker_brightness=value.int_value;
    printf("picker_brightness_callback: value: %f\n", picker_brightness);
}
void picker_hue_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg){
    if (value.format != homekit_format_float) {
        printf("Invalid picker_hue_callback-value format: %d\n", value.format);
        return;
    }
    picker_hue=value.float_value;
    printf("picker_hue_callback: value: %f\n", picker_hue);
}
void picker_saturation_callback(homekit_characteristic_t *ch, homekit_value_t value, void *arg){
    if (value.format != homekit_format_float) {
        printf("Invalid picker_saturation_callback-value format: %d\n", value.format);
        return;
    }
    picker_saturation=value.float_value;
    printf("picker_saturation_callback: value: %f\n", picker_saturation);
}
*/

/*
 * Homekit Accessories
 */
homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id = 1, .category = homekit_accessory_category_television, .services = (homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics = (homekit_characteristic_t*[]) {
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "amaider"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "137A2BABF19D"),
            HOMEKIT_CHARACTERISTIC(MODEL, "LED Tiles"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, NULL),
            NULL
        }),
        /*HOMEKIT_SERVICE(LIGHTBULB, .primary = true, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Color Picker"),
            HOMEKIT_CHARACTERISTIC(ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(picker_on_callback)),
            HOMEKIT_CHARACTERISTIC(BRIGHTNESS, 0, .max_value = (float[]) {LED_COUNT}, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(picker_brightness_callback)),
            HOMEKIT_CHARACTERISTIC(HUE, 0, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(picker_hue_callback)),
            HOMEKIT_CHARACTERISTIC(SATURATION, 0, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(picker_saturation_callback)),
            NULL
        }),*/
        HOMEKIT_SERVICE(TELEVISION, .primary = true, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(ACTIVE, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(tv_active_callback)),        
            HOMEKIT_CHARACTERISTIC(ACTIVE_IDENTIFIER, 1, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(tv_active_identifier_callback)),
            &tv_configured_name,
            //HOMEKIT_CHARACTERISTIC(CONFIGURED_NAME, "tvtv"),
            HOMEKIT_CHARACTERISTIC(SLEEP_DISCOVERY_MODE, HOMEKIT_SLEEP_DISCOVERY_MODE_ALWAYS_DISCOVERABLE),
            HOMEKIT_CHARACTERISTIC(POWER_MODE_SELECTION, HOMEKIT_POWER_MODE_SELECTION_SHOW, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(tv_power_mode_selection_callback)),
            HOMEKIT_CHARACTERISTIC(REMOTE_KEY, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(tv_remote_key_callback)),
            NULL
        }, .linked=(homekit_service_t*[]) {
            &input_source1,
            &input_source2,
            &tv_speaker,
            NULL
        }),
        &input_source1,
        &input_source2,
        &tv_speaker,
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111",
    .setupId = "1JH0",  //maybe change? to JH10
};

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    int name_len = snprintf(NULL, 0, "amaider-%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len + 1);
    snprintf(name_value, name_len + 1, "amaider-%02X%02X%02X", macaddr[3], macaddr[4], macaddr[5]);
    name.value = HOMEKIT_STRING(name_value);
}

void on_wifi_ready() {
    homekit_server_init(&config);  //replace on_wifi_ready
}

void user_init(void) {
    create_accessory_name();
    //wifi_config_init("amaider - LED Tiles ", NULL, on_wifi_ready);    //replace NULL with passwd
    WS2812FX_init(LED_COUNT);

    wifi_init();
    homekit_server_init(&config);
}