QEMU=./Qemu/src/arm-softmmu/qemu-system-arm
HOST=127.0.1.1
APP1=Demonstrations/Demo1_LEDs
APP2=Demonstrations/Demo2_Buttons

QFLAGS=-M stm32l152rbt6 -nographic
QFLAGS+=-chardev socket,id=led_green,port=4242,host=$(HOST),nodelay
QFLAGS+=-chardev socket,id=led_blue,port=4243,host=$(HOST),nodelay
QFLAGS+=-chardev socket,id=user0,port=4244,host=$(HOST),nodelay

all: qemu app1 app2

qemu: Qemu/src/arm-softmmu/qemu-system-arm
app1: ./$(APP1)/main_qemu.elf
app2: ./$(APP2)/main_qemu.elf

Qemu/src/arm-softmmu/qemu-system-arm:
	cd Qemu/src && ./configure --target-list=arm-softmmu && make


./$(APP1)/main_qemu.elf ./$(APP2)/main_qemu.elf:
	cd `dirname $@` && make

run1:
	$(QEMU) $(QFLAGS) -kernel $(APP1)/main_qemu.elf

run2:
	$(QEMU) $(QFLAGS) -kernel $(APP2)/main_qemu.elf

mrproper clean:
	cd $(APP1) && make $@
	cd $(APP2) && make $@
