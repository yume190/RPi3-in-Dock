cmd_/build/chardev.ko := arm-linux-gnueabihf-ld -EL -r  -T ./scripts/module-common.lds --build-id  -o /build/chardev.ko /build/chardev.o /build/chardev.mod.o
