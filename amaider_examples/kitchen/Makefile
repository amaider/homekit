PROGRAM = kitchen

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/i2c \
	extras/bmp280 \
	$(abspath ../../esp-homekit-demo/components/common/button) \
	$(abspath ../../esp-homekit-demo/components/esp8266-open-rtos/cJSON) \
	$(abspath ../../esp-homekit-demo/components/common/wolfssl) \
	$(abspath ../../esp-homekit-demo/components/common/homekit)

FLASH_SIZE ?= 32
#FLASH_SIZE ?= 8
#HOMEKIT_SPI_FLASH_BASE_ADDR ?= 0x7A000
#FLASH_MODE=dout

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS
#EXTRA_CFLAGS += -DHOMEKIT_DEBUG=1
include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
