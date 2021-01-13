/*
WS2812FX.h - Library for WS2812 LED effects.
06.05.2018: https://github.com/kitesurfer1404/WS2812FX/tree/6ce37240ec411b24baa0444597bb88d828555569/src


Ported to esp-open-rtos by PCSaito - 2018
07.05.2018: https://github.com/pcsaito/WS2812FX-rtos

modified by amaider - 2020
*/

#ifndef ledtiles_WS2812FX_h
#define ledtiles_WS2812FX_h

#include "ws2812_i2s/ws2812_i2s.h"

//#define LED_INBUILT_GPIO 2      // this is the onboard LED used to show on/off only

#define DEFAULT_MODE 0
#define DEFAULT_SPEED 1
//#define DEFAULT_COLOR 0xFF10EE
#define DEFAULT_COLOR WHITE

#define RED 			0xFF0000
#define ORANGE			0xFF8000
#define YELLOW			0xFFFF00
#define LIGHT_GREEN		0x80FF00
#define GREEN			0x00FF00
#define GREEN_BLUE_MIX	0x00FF80
#define CYAN			0x00FFFF
#define LIGHT_BLUE		0x0080FF
#define BLUE			0x0000FF
#define PURPLE			0x8000FF
#define MAGENTA			0xFF00FF
#define PINK			0xFF0080

#define WHITE			0xFFFFFF
#define BLACK			0x000000
#define TIGER_CYAN		0x27bce9
#define TIGER_MAGENTA	0xc80dbd
#define TIGER_GREEN		0x4c7504

#define LED_COUNT 18

#define SPEED_MIN 1
#define SPEED_MAX 255
#define SPEED_STEP 10

#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 255
#define BRIGHTNESS_STEP 10
#define BRIGHTNESS_FILTER 0.9	// affects only if(_slow_start)

#define HUE_STEP 30
#define SATURATION_STEP 20

#define MODE_COUNT 9 //	0-8

#define FX_MODE_STATIC					0
#define FX_MODE_COLORWIPE				1
#define FX_MODE_RAINBOW					2
#define FX_MODE_RAINBOW_SIDEWAYS		3
#define FX_MODE_RAINBOW_SIDEWAYS_BIG	4
#define FX_MODE_CUSTOM_STATIC			5
#define FX_MODE_CUSTOM_WIPE				6
#define FX_MODE_CUSTOM_RAINBOW			7
#define FX_MODE_EDITMODE				8

typedef void (*mode)(void);
  
void
	WS2812FX_init(uint16_t pixel_count),
	WS2812FX_initModes(void),
	WS2812FX_service(void *_args),
	WS2812FX_start(void),
	WS2812FX_stop(void),
	WS2812FX_setMode(uint8_t m),
	WS2812FX_setMode360(float m),
	WS2812FX_setSpeed(uint8_t s),
	WS2812FX_setColor(uint8_t r, uint8_t g, uint8_t b),
	WS2812FX_setColor32(uint32_t c),
	WS2812FX_setBrightness(uint8_t b),
	WS2812FX_setInverted(bool inverted),
	WS2812FX_setSlowStart(bool slow_start),
	hsi2rgb(float h, float s, float i, ws2812_pixel_t* rgb);

bool
	WS2812FX_isRunning(void);

uint8_t
	WS2812FX_getMode(void),
	WS2812FX_getSpeed(void),
	WS2812FX_getBrightness(void),
	WS2812FX_getModeCount(void);

uint16_t
	WS2812FX_getLength(void);

uint32_t
	WS2812FX_color_wheel(uint16_t),
	WS2812FX_getColor(void);

//private
void
	WS2812FX_strip_off(void),
	WS2812FX_mode_static(void),
	WS2812FX_mode_colorwipe(void),
	WS2812FX_mode_rainbow(void),
	WS2812FX_mode_rainbow_sideways(void),
	WS2812FX_mode_rainbow_sideways_big(void),
	WS2812FX_mode_custom_static(void),
	WS2812FX_mode_custom_wipe(void),
	WS2812FX_mode_custom_rainbow(void),
	WS2812FX_mode_editmode(void);

#endif
