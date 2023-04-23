Build/kernel.elf :	Build/main.elf Build/start.elf Build/kout.elf Build/trap.elf Build/trapEntry.elf Build/clock.elf Build/interrupt.elf
	riscv64-unknown-elf-ld -o Build/kernel.elf -T Linker/linker.ld Build/main.elf Build/start.elf Build/kout.elf Build/trap.elf Build/trapEntry.elf Build/clock.elf Build/interrupt.elf

#kernel
Build/start.elf:kernel/boot/start.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/boot/start.S -o Build/start.elf -I"kernel/include" -mcmodel=medany

Build/main.elf:kernel/boot/main.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/boot/main.cpp -o Build/main.elf -I"kernel/include" -mcmodel=medany

Build/kout.elf:kernel/lib/kout.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/lib/kout.cpp -o Build/kout.elf -I"kernel/include" -mcmodel=medany

#trap
Build/trap.elf:kernel/trap/trap.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/trap.cpp -o Build/trap.elf -I"kernel/include" -mcmodel=medany

Build/trapEntry.elf:kernel/trap/trapEntry.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/trapEntry.S -o Build/trapEntry.elf -I"kernel/include" -mcmodel=medany

Build/clock.elf:kernel/trap/clock.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/clock.cpp -o Build/clock.elf -I"kernel/include" -mcmodel=medany

Build/interrupt.elf:kernel/trap/interrupt.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/interrupt.cpp -o Build/interrupt.elf -I"kernel/include" -mcmodel=medany


clean:
	rm Build/*

qemu:
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios SBI/opensbi-qemu.elf -kernel Build/kernel.elf