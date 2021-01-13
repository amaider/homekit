# Build instructions ESP8266 (Docker)

(Maxim Kulkin edited this page on 10 Nov 2020 · 6 revisions)

Here is build setup that I use. Note that you need Docker, python2.7 and python `pip` tool installed.

1. Create an empty directory and change into it.
2. Create a file `esp-sdk-dockerfile` with following content:
```Docker
FROM ubuntu:18.04 as builder

ENV DEBIAN_FRONTEND=noninteractive

RUN groupadd -g 1000 docker && useradd docker -u 1000 -g 1000 -s /bin/bash --no-create-home
RUN mkdir /build && chown docker:docker /build

RUN apt-get update && apt-get install -y \
  make unrar-free autoconf automake libtool gcc g++ gperf \
  flex bison texinfo gawk ncurses-dev libexpat-dev python-dev python python-serial \
  sed git unzip bash help2man wget bzip2 libtool-bin

RUN su docker -c " \
    git clone --recursive https://github.com/pfalcon/esp-open-sdk.git /build/esp-open-sdk ; \
    cd /build/esp-open-sdk ; \
    make STANDALONE=n ; \
"


FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y make python python-serial

COPY --from=builder /build/esp-open-sdk/xtensa-lx106-elf /opt/xtensa-lx106-elf
ENV PATH /opt/xtensa-lx106-elf/bin:$PATH
```
3. Create a file `esp-rtos-dockerfile` with following content:
```Docker
FROM ubuntu:18.04 as builder

RUN apt-get update && apt-get install -y git

RUN git clone --recursive https://github.com/Superhouse/esp-open-rtos.git /opt/esp-open-rtos


FROM esp-sdk:latest

COPY --from=builder /opt/esp-open-rtos /opt/esp-open-rtos

ENV SDK_PATH /opt/esp-open-rtos
```
4. Build `esp-sdk` Docker container:
```shell
docker build . -f esp-sdk-dockerfile -t esp-sdk
```
5. Build `esp-rtos` Docker container:
```shell
docker build . -f esp-rtos-dockerfile -t esp-rtos
```
6. Clone `esp-open-rtos` repository:
```shell
git clone --recursive https://github.com/SuperHouse/esp-open-rtos.git
```
7. Install esptool.py:
```shell
pip install esptool
```
8. Clone `esp-homekit-demo` repository:
```shell
git clone --recursive https://github.com/maximkulkin/esp-homekit-demo.git
```
9. Setup enviroment variables:
```shell
export SDK_PATH="$(pwd)/esp-open-rtos"
export ESPPORT=/dev/tty.SLAB_USBtoUART
```
To find out what is the name of your USB device to put to ESPPORT environment variable, first do `ls /dev/tty.*` before you connect your ESP8266 to USB, then do same command after you have connected it to USB and notice which new device has appeared.

10. Copy wifi.h.sample -> wifi.h and edit it with correct WiFi SSID and password.
11. [Configure settings](https://github.com/maximkulkin/esp-homekit-demo/wiki/Configuration)
12. To build an example", first change into `esp-homekit-demo` directory (into it's root directory:
```shell
cd esp-homekit-demo
```
Then build example you want (e.g. `sonoff_basic`) by running
```shell
docker run -it --rm -v "$(pwd)":/project -w /project esp-rtos make -C examples/sonoff_basic all
```
Then flash it (and optionally immediately run monitor)
```shell
make -C examples/sonoff_basic flash monitor
```
NOTE: personally I do a lot of stuff in Docker containers, so I have following helper function in my ~/.bashrc:
```shell
docker-run() {
  docker run -it --rm -v "$(pwd)":/project -w /project "$@"
}
```
Then, to run a container I just do
```shell
docker-run esp-rtos make -C examples/sonoff_basic all
```
