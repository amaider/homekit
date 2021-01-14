PROGRAM = PIR

EXTRA_COMPONENTS = \
	extras/http-parser \
	$(abspath ../../esp-homekit-demo/components/esp8266-open-rtos/cJSON) \
	$(abspath ../../esp-homekit-demo/components/common/button) \
	$(abspath ../../esp-homekit-demo/components/common/wolfssl) \
	$(abspath ../../esp-homekit-demo/components/common/homekit)

FLASH_SIZE ?= 32

SENSOR_PIN ?= 0

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DSENSOR_PIN=$(SENSOR_PIN)

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
