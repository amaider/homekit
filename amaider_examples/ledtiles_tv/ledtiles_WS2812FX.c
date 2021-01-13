/*
WS2812FX.cpp - Library for WS2812 LED effects.
06.05.2018: https://github.com/kitesurfer1404/WS2812FX/tree/6ce37240ec411b24baa0444597bb88d828555569/src


Ported to esp-open-rtos by PCSaito - 2018
www.github.com/pcsaito
07.05.2018: https://github.com/pcsaito/WS2812FX-rtos

modified by amaider - 2020
*/

#include "ledtiles_WS2812FX.h"
#include <math.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#define CALL_MODE(n) _mode[n]();

#define debug(fmt, ...) printf("%s" fmt "\n", "ledtiles_WS2812FX.c: ", ## __VA_ARGS__);

/*
bool led_on = false;            // on is boolean on or off
float led_brightness = 50;     // brightness is scaled 0 to 100
float led_hue = 0;              // hue is scaled 0 to 360
float led_saturation = 100;      // saturation is scaled 0 to 100

bool picker_on = false;
float picker_brightness = 0;
float picker_hue = 0;
float picker_saturation = 100;

bool switch_bool = false;
uint16_t current_source = DEFAULT_MODE;
uint8_t current_mode = DEFAULT_MODE;
uint16_t customHue[25] = {360,0,240,120,333,102,87,302,90,9,9,0,0,0,216,230,245,259,274,288,302,317,331,346,360};
uint16_t customSaturation[25] = {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100};*/
char *modename[8] = {"Static", "Colorwipe", "Rainbow", "Rainbow Sideways", "Rainbow Sideways Big", "Custom Static", "Custom Wipe", "Custom Rainbow"};

bool is_paused = false;

uint8_t current_mode = DEFAULT_MODE;
uint8_t current_speed = DEFAULT_SPEED;
uint16_t current_hue = 200;
uint8_t current_saturation = 100;
uint16_t current_brightness = 255;

bool editMode = false;
bool is_selected = false;
uint8_t selected_led = 0;
uint16_t selected_hue = 0;
uint8_t selected_saturation = 100;
uint16_t selected_brightness = 255;
uint16_t customHue[LED_COUNT];
uint8_t customSaturation[LED_COUNT];
uint8_t customBrightness[LED_COUNT];
uint8_t custom_wipe_number = LED_COUNT;


uint8_t _mode_index = DEFAULT_MODE;
uint8_t _speed = DEFAULT_SPEED;
uint8_t _brightness = 0;
uint8_t _target_brightness = 0;
bool _running = false;
bool _inverted = false;
bool _slow_start = false;

uint16_t _led_count = 0;

uint32_t _color = DEFAULT_COLOR;
uint32_t _mode_color = DEFAULT_COLOR;

uint32_t _mode_delay = 100;
uint32_t _counter_mode_call = 0;
uint32_t _counter_mode_step = 0;
uint32_t _mode_last_call_time = 0;
	  
uint8_t get_random_wheel_index(uint8_t);

mode _mode[MODE_COUNT];

ws2812_pixel_t *pixels;

//Helpers
uint32_t color32(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

uint32_t constrain(uint32_t amt, uint32_t low, uint32_t high) {
	return (amt < low) ? low : ((amt > high) ? high : amt);
}

float fconstrain(float amt, float low, float high) {
	return (amt < low) ? low : ((amt > high) ? high : amt);
}

uint32_t randomInRange(uint32_t min, uint32_t max) {
	if (min < max) {
		uint32_t randomValue = rand() % (max - min);
		return randomValue + min;
	} else if (min == max) {
		return min;
	}
	return 0;
}

long  map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static uint16_t min(uint16_t a, uint16_t b) {
    return (a > b) ? b : a;
}

static uint32_t max(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

//LED Adapter
void WS2812_show(void) {
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void WS2812_setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
	if (_inverted) { 
		n = (_led_count - 1) - n; 
	}
	
	ws2812_pixel_t px;
	px.red = map(r, 0, BRIGHTNESS_MAX, BRIGHTNESS_MIN, _brightness);
	px.green = map(g, 0, BRIGHTNESS_MAX, BRIGHTNESS_MIN, _brightness);
	px.blue = map(b, 0, BRIGHTNESS_MAX, BRIGHTNESS_MIN, _brightness);
	
	pixels[n] = px;
}

void WS2812_setPixelColor32(uint16_t n, uint32_t c) {
	uint8_t r = (uint8_t)(c >> 16);
	uint8_t g = (uint8_t)(c >>  8);
	uint8_t b = (uint8_t)c;
	
	WS2812_setPixelColor(n, r, g, b);
}

uint32_t WS2812_getPixelColor(uint16_t n) {
	return color32(pixels[n].red, pixels[n].green, pixels[n].blue);
}

void WS2812_clear() {
	for (uint16_t i = 0; i < _led_count; i++) {
		WS2812_setPixelColor(i, 0, 0, 0);
	}
	ws2812_i2s_update(pixels, PIXEL_RGB);
}

void WS2812_init(uint16_t pixel_count) {
	_led_count = pixel_count;
    pixels = (ws2812_pixel_t*) malloc(_led_count * sizeof(ws2812_pixel_t));
	
	// initialise the onboard led as a secondary indicator (handy for testing)
	// gpio_enable(LED_INBUILT_GPIO, GPIO_OUTPUT);

	// initialise the LED strip
	ws2812_i2s_init(_led_count, PIXEL_RGB);
	
	WS2812_clear();
}

//WS2812FX
void WS2812FX_init(uint16_t pixel_count) {
	WS2812_init(pixel_count);
	xTaskCreate(WS2812FX_service, "fxService", 255, NULL, 2, NULL);
	WS2812FX_initModes();
	WS2812FX_start();
}

void WS2812FX_service(void *_args) {
	uint32_t now = 0;
	
	while (true) {
		if(_running) {
			//printf("_brightness : _target_brightness %ld : %ld \n", _brightness, _target_brightness);
			
            if (_slow_start) {
			    if ((_brightness < _target_brightness)) {
                    uint8_t new_brightness = (BRIGHTNESS_FILTER * _brightness) + ((1.0-BRIGHTNESS_FILTER) * _target_brightness);
                    float soft_start = fconstrain((float)(_brightness * 4) / (float)BRIGHTNESS_MAX, 0.1, 1.0);
                    uint8_t delta = (new_brightness - _brightness) * soft_start;
	                _brightness = _brightness + constrain(delta, 1, delta);
	            } else {
	                _brightness = (BRIGHTNESS_FILTER * _brightness) + ((1.0-BRIGHTNESS_FILTER) * _target_brightness);
	            }
            } else {
                _brightness = _target_brightness;
            }
		
			now = xTaskGetTickCount() * portTICK_PERIOD_MS;
			if(now - _mode_last_call_time > _mode_delay) {
				_counter_mode_call++;
				_mode_last_call_time = now;
				CALL_MODE(_mode_index);
				
				//gpio_toggle(LED_INBUILT_GPIO); //led indicator
			}  
		}
		vTaskDelay(33 / portTICK_PERIOD_MS);
	}
}

void WS2812FX_start() {
	_counter_mode_call = 0;
	_counter_mode_step = 0;
	_running = true;
}

void WS2812FX_stop() {
	_running = false;
}

void WS2812FX_setMode360(float m) {
	//printf("WS2812FX_setMode360: %f", m);
	uint8_t mode = map((uint16_t)m, 0, 360, 0, MODE_COUNT-1);
	//printf("WS2812FX_setMode: %d", mode);
	WS2812FX_setMode(mode);
}

void WS2812FX_setMode(uint8_t m) {
	_counter_mode_call = 0;
	_counter_mode_step = 0;
	_mode_index = constrain(m, 0, MODE_COUNT-1);
	_mode_color = _color;
}

void WS2812FX_setSpeed(uint8_t s) {
	_counter_mode_call = 0;
	_counter_mode_step = 0;
	_speed = constrain(s, SPEED_MIN, SPEED_MAX);
}

void WS2812FX_setColor(uint8_t r, uint8_t g, uint8_t b) {
	WS2812FX_setColor32(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

void WS2812FX_setColor32(uint32_t c) {
	_color = c;
	_counter_mode_call = 0;
	_counter_mode_step = 0;
	_mode_color = _color;
}

void WS2812FX_setBrightness(uint8_t b) {
	_target_brightness = constrain(b, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
	//printf("###WS2812FX_setBrightness: %d \n", _target_brightness);
}

void WS2812FX_forceBrightness(uint8_t b) {
	_target_brightness = constrain(b, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
	_brightness = _target_brightness;
	//printf("###WS2812FX_forceBrightness: %d \n", _brightness);
}
void WS2812FX_forceMode(uint8_t m) {
	_counter_mode_call = 0;
	_counter_mode_step = 0;
	_mode_index = constrain(m, 0, MODE_COUNT-1);
	_mode_color = _color;
}

bool WS2812FX_isRunning() {
	return _running;
}

uint8_t WS2812FX_getMode(void) {
	return _mode_index;
}

uint8_t WS2812FX_getSpeed(void) {
	return _speed;
}

uint8_t WS2812FX_getBrightness(void) {
	return _target_brightness;
}

uint16_t WS2812FX_getLength(void) {
	return _led_count;
}

uint8_t WS2812FX_getModeCount(void) {
	return MODE_COUNT;
}

uint32_t WS2812FX_getColor(void) {
	return _color;
}

void WS2812FX_setInverted(bool inverted) {
	_inverted = inverted;
}

void WS2812FX_setSlowStart(bool slow_start) {
	_slow_start = slow_start;
}

//http://blog.saikoled.com/post/44677718712/how-to-convert-from-hsi-to-rgb-white
void hsi2rgb(float h, float s, float i, ws2812_pixel_t* rgb) {
	#define LED_RGB_SCALE 255			// this is the scaling factor used for color conversion
    int r, g, b;

    while (h < 0) { h += 360.0F; };     // cycle h around to 0-360 degrees
    while (h >= 360) { h -= 360.0F; };
    h = 3.14159F*h / 180.0F;            // convert to radians.
    s /= 100.0F;                        // from percentage to ratio
    i /= 100.0F;                        // from percentage to ratio
    s = s > 0 ? (s < 1 ? s : 1) : 0;    // clamp s and i to interval [0,1]
    i = i > 0 ? (i < 1 ? i : 1) : 0;    // clamp s and i to interval [0,1]
    i = i * sqrt(i);                    // shape intensity to have finer granularity near 0

    if (h < 2.09439) {
        r = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        g = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        b = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else if (h < 4.188787) {
        h = h - 2.09439;
        g = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        b = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        r = LED_RGB_SCALE * i / 3 * (1 - s);
    }
    else {
        h = h - 4.188787;
        b = LED_RGB_SCALE * i / 3 * (1 + s * cos(h) / cos(1.047196667 - h));
        r = LED_RGB_SCALE * i / 3 * (1 + s * (1 - cos(h) / cos(1.047196667 - h)));
        g = LED_RGB_SCALE * i / 3 * (1 - s);
    }

    rgb->red = (uint8_t) r;
    rgb->green = (uint8_t) g;
    rgb->blue = (uint8_t) b;
    rgb->white = (uint8_t) 0;           // white channel is not used
	debug("r=%i, g=%i, b=%i", r, g, b);
}

/*
 * Color and Blinken Functions
 */

/*
 * Turns everything off. Doh.
 */
void WS2812FX_strip_off() {
	WS2812_clear();
}

/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r
 * Inspired by the Adafruit examples.
 */
uint32_t WS2812FX_color_wheel(uint16_t pos) {
	pos = 255 - pos;
	if(pos<0){
		pos = pos * (-1);
	}
	if(pos < 85) {
		return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
	} else if(pos < 170) {
		pos -= 85;
		return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
	} else {
		pos -= 170;
		return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
	}
}

/*
 * Returns a new, random wheel index with a minimum distance of 42 from pos.
 */
uint8_t WS2812FX_get_random_wheel_index(uint8_t pos) {
	uint8_t r = 0;
	uint8_t x = 0;
	uint8_t y = 0;
	uint8_t d = 0;

	while(d < 42) {
		r = randomInRange(0, 256);
		x = abs(pos - r);
		y = 255 - x;
		d = min(x, y);
	}

	return r;
}

/*
 * declaration for functions for modes
 */
static void _static(uint32_t c);
static void _colorwipeHEX(uint32_t c);
static void _colorwipeRGB(uint8_t r, uint8_t g, uint8_t b);
static void _rainbow();
static void _rainbow_sideways();
static void _custom_static_rgb();
static void _custom_rainbow();
static void _rainbow_sideways_big();
static void _editmode();

/*
 * 9 Modes
 */
void WS2812FX_mode_static(void) {
	/*for(uint16_t i=0; i<_led_count; i++) {	// full power saving test (Ampere)
		WS2812_setPixelColor32(i, WHITE);
	}
	WS2812_show();
	vTaskDelay(100 / portTICK_PERIOD_MS);*/
	
	ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };
	hsi2rgb(current_hue, current_saturation, current_brightness, &rgb);
	//hsi2rgb(customHue[1], customSaturation[1], current_brightness, &rgb);
	for(uint16_t i=0; i<_led_count; i++) {
		WS2812_setPixelColor(i, rgb.red, rgb.green, rgb.blue);
	}
	WS2812_show();
	vTaskDelay(100 / portTICK_PERIOD_MS);
}
void WS2812FX_mode_colorwipe(void) {
	/*_colorwipeHEX(RED);
	_colorwipeHEX(ORANGE);
	_colorwipeHEX(GREEN);
	_colorwipeHEX(CYAN);
	_colorwipeHEX(BLUE);
	_colorwipeHEX(PINK);*/
	_colorwipeHEX(WHITE);
}
void WS2812FX_mode_rainbow(void) {
	_rainbow();
}
void WS2812FX_mode_rainbow_sideways(void) {
	_rainbow_sideways();
}
void WS2812FX_mode_rainbow_sideways_big(void) {
	_rainbow_sideways_big();
}
void WS2812FX_mode_custom_static(void) {
	_custom_static_rgb();
}
void WS2812FX_mode_custom_wipe(void) {
	/*custom_wipe_number = LED_COUNT;
	for(uint16_t i=0; i<custom_wipe_number; i++){
		ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };
		hsi2rgb(customHue[i], customSaturation[i], customBrightness[i], &rgb);
		_colorwipeRGB(rgb.red, rgb.green, rgb.blue);
		if(customHue[i] == customHue[i+1]){
			i=custom_wipe_number;
		}
	}*/
}
void WS2812FX_mode_custom_rainbow(void) {
	_custom_rainbow();
}
void WS2812FX_mode_editmode(void) {
	_editmode();
}

/*
 * functions for modes
 */
void _static(uint32_t c) {
	for(uint16_t i=0; i<_led_count; i++) {
		WS2812_setPixelColor32(i, c);
	}
	WS2812_show();
	_mode_delay = 50;	
}
void _colorwipeHEX(uint32_t c) {
	for(uint16_t i=0; i<_led_count; i++) {
		WS2812_setPixelColor32(i, c);
		WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
		WS2812_show();
		vTaskDelay(100 / portTICK_PERIOD_MS);
  	}
}
void _rainbow(){
	for(uint16_t j=0; j<256*1; j++) {	//256*1 = 1cycle
		for(uint16_t i=0; i < _led_count; i++) {
			WS2812_setPixelColor32(i, WS2812FX_color_wheel((j) & 255));
    	}
		WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
    	WS2812_show();
    	vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
void _rainbow_sideways(){
	for(uint16_t j=0; j<256*1; j++) {	//256*1 = 1cycle
		for(uint16_t i=0; i < _led_count; i++) {
			uint16_t c = 255/_led_count;
			WS2812_setPixelColor32(i, WS2812FX_color_wheel((255-(i*c)+j) & 255));	// 255- for reverse, r-b-g
    	}
		WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
    	WS2812_show();
    	vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
void _rainbow_sideways_big() {
	for(uint16_t j=0; j<256*1; j++) {	//256*1 = 1cycle
		for(uint16_t i=0; i < _led_count; i++) {
			float c = i * (255.0/_led_count)* (1.0/6);	// 255 color spectrum on _led_count, 255*(1/6) color spectrum on _led_count
			WS2812_setPixelColor32(i, WS2812FX_color_wheel(((uint16_t)c+j) & 255));
    	}
		WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
    	WS2812_show();
    	vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
void _custom_static_rgb(){
	for(uint16_t i=0; i<_led_count; i++){
		ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };
    	hsi2rgb(customHue[i], customSaturation[i], customBrightness[i], &rgb);
		WS2812_setPixelColor(i, rgb.red, rgb.green, rgb.blue);
	}
	WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
	WS2812_show();
	vTaskDelay(100 / portTICK_PERIOD_MS);
}
void _colorwipeRGB(uint8_t r, uint8_t g, uint8_t b){
	for(uint16_t i=0; i<_led_count; i++) {
		WS2812_setPixelColor(i, r, g, b);
		WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
		WS2812_show();
		vTaskDelay(100 / portTICK_PERIOD_MS);
  	}
}
void _custom_rainbow(){
	for(uint16_t j=0; j<360*1; j++) {
		for(uint16_t i=0; i<_led_count; i++) {
			ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };
			hsi2rgb((customHue[i]+j) % 360, customSaturation[i], customBrightness[i], &rgb);
			WS2812_setPixelColor(i, rgb.red, rgb.green, rgb.blue);
		}
		WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
		WS2812_show();
    	vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
void _editmode(){
	if(!is_selected) {
		if(_counter_mode_call % 2 == 1) {
			for(uint8_t i=0; i < _led_count; i++) {
				ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };
    			hsi2rgb(customHue[i], customSaturation[i], customBrightness[i], &rgb);
				WS2812_setPixelColor(i, rgb.red, rgb.green, rgb.blue);
			}
			_mode_delay = 500;
		} else {
			WS2812_setPixelColor(selected_led, 0, 0, 0);
			_mode_delay = 500;
		}
	}else if(is_selected) {
		WS2812FX_forceBrightness(selected_brightness/5);
		for(uint8_t i=0; i<_led_count; i++) {
			ws2812_pixel_t rgb = { { 0, 0, 0, 0 } };
    		hsi2rgb(customHue[i], customSaturation[i], customBrightness[i], &rgb);
			WS2812_setPixelColor(i, rgb.red, rgb.green, rgb.blue);
		}
		WS2812FX_forceBrightness(selected_brightness);
		ws2812_pixel_t rgb2 = { { 0, 0, 0, 0 } };
    	hsi2rgb(selected_hue, selected_saturation, selected_brightness, &rgb2);
		WS2812_setPixelColor(selected_led, rgb2.red, rgb2.green, rgb2.blue);
	}
	//WS2812FX_forceBrightness((uint8_t)floor(current_brightness));
	WS2812_show();
	vTaskDelay(100 / portTICK_PERIOD_MS);
}



void WS2812FX_initModes() {
	_mode[FX_MODE_STATIC]					= &WS2812FX_mode_static;
	_mode[FX_MODE_COLORWIPE]				= &WS2812FX_mode_colorwipe;
	_mode[FX_MODE_RAINBOW]					= &WS2812FX_mode_rainbow;
	_mode[FX_MODE_RAINBOW_SIDEWAYS]			= &WS2812FX_mode_rainbow_sideways;
	_mode[FX_MODE_CUSTOM_STATIC]			= &WS2812FX_mode_custom_static;
	_mode[FX_MODE_CUSTOM_WIPE]				= &WS2812FX_mode_custom_wipe;
	_mode[FX_MODE_CUSTOM_RAINBOW]			= &WS2812FX_mode_custom_rainbow;
	_mode[FX_MODE_RAINBOW_SIDEWAYS_BIG]		= &WS2812FX_mode_rainbow_sideways_big;
	_mode[FX_MODE_EDITMODE]					= &WS2812FX_mode_editmode;

}