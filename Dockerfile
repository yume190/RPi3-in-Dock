# DOCKER-VERSION 1.0.0

FROM debian:jessie
MAINTAINER yume190 <yume190@gmail.com>

ENV KERNEL kernel7
ENV PATH ${PATH}:/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
ENV VERSION 4
ENV PATCHLEVEL 4
ENV SUBLEVEL 12
ENV KERNEL_DIR /kdir
ENV KERNEL_IMAGE /kimg
ENV KERNEL_BUILD_DIR $KERNEL_DIR/lib/modules/$VERSION.$PATCHLEVEL.$SUBLEVEL-v7+/build

RUN apt-get update -y && \
    apt-get install -y libncurses5-dev git make gcc bc && \
    apt-get clean -y && \
    git clone --depth=1 https://github.com/raspberrypi/linux && \
    git clone --depth=1 https://github.com/raspberrypi/tools

COPY ./config/$VERSION.$PATCHLEVEL.$SUBLEVEL.config /linux/.config

RUN cd /linux && \
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j4 zImage modules dtbs

RUN cd /linux && \
    mkdir $KERNEL_IMAGE && \
    mkdir $KERNEL_DIR && \
    make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- INSTALL_MOD_PATH=$KERNEL_DIR modules_install
# scripts/mkknlimg arch/arm/boot/zImage $KERNEL_IMAGE/$KERNEL.img && \
# cp arch/arm/boot/dts/*.dtb $KERNEL_IMAGE/ && \
# cp arch/arm/boot/dts/overlays/*.dtb* $KERNEL_IMAGE/overlays/ && \
# cp arch/arm/boot/dts/overlays/README $KERNEL_IMAGE/overlays/


