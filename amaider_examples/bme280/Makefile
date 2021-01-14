PROGRAM = bme280

EXTRA_COMPONENTS = \
	extras/i2c \
	extras/bmp280 \
	extras/http-parser \
	$(abspath ../../esp-homekit-demo/components/esp8266-open-rtos/cJSON) \
	$(abspath ../../esp-homekit-demo/components/common/wolfssl) \
	$(abspath ../../esp-homekit-demo/components/common/homekit)

FLASH_SIZE ?= 32

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)