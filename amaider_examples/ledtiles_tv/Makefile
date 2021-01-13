PROGRAM = ledtiles_tv

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/dhcpserver \
	extras/i2s_dma \
	extras/ws2812_i2s \
	$(abspath ../../esp-homekit-demo/components/esp8266-open-rtos/wifi_config) \
	$(abspath ../../esp-homekit-demo/components/esp8266-open-rtos/cJSON) \
	$(abspath ../../esp-homekit-demo/components/common/wolfssl) \
	$(abspath ../../esp-homekit-demo/components/common/homekit)

FLASH_SIZE ?= 32
# FLASH_SIZE ?= 8
HOMEKIT_SPI_FLASH_BASE_ADDR=0x7A000

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS

#EXTRA_CFLAGS += -DHOMEKIT_DEBUG=1

include $(SDK_PATH)/common.mk
#include $(abspath ../../wifi.h)

LIBS += m

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)