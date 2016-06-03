cmd_/build/hello-1.ko := arm-linux-gnueabihf-ld -EL -r  -T ./scripts/module-common.lds --build-id  -o /build/hello-1.ko /build/hello-1.o /build/hello-1.mod.o
