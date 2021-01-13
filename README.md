# esp-homekit-devices


# Build Guide:
```shell
docker build . -f esp-sdk-dockerfile -t esp-sdk
```
```shell
docker build . -f esp-rtos-dockerfile -t esp-rtos
```
```shell
git clone --recursive https://github.com/SuperHouse/esp-open-rtos.git
```
```shell
pip install esptool
```
```shell
git clone --recursive https://github.com/maximkulkin/esp-homekit-demo.git
```
Find port = `ls /dev/tty.*`
add PATHs to ~/.bash_profile
```shell
export SDK_PATH="$(pwd)/esp-open-rtos"
export ESPPORT=/dev/tty.SLAB_USBtoUART
```
```shell
docker run -it --rm -v "$(pwd)":/project -w /project esp-rtos make -C examples/sonoff_basic all
```
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