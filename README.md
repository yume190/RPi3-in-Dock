Cross Compile Raspberry Pi 3
=================

### Target

 * 裝置： Raspberry Pi 2/3
 * kernel： 4.4.12
   修改版本好可以參考 [raspberrypi/linux/Makefile](https://github.com/raspberrypi/linux/blob/rpi-4.4.y/Makefile#L1,L3)
 * `.config`： 為了盡可能與裝置的 kernel 相近，最好將 `.config` 導出。
       可以用 `sudo modprobe configs` 和 `zcat /proc/config.gz > .config` 指令
       目前 `.config` 放在 `config/4.4.12.config`


### Build

    docker build -t rasp .

### Run

    docker run -it --rm -v ./build:/build rasp

### Other

剩下可以參考

 * [Kbuild](build/Kbuild)
 * [Makefile](build/Makefile)
 * [Dockerfile](Dockerfile)
