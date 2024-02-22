.PHONY: all clean

all:
	$(MAKE) -C bootloader_uefi

	dd if=/dev/zero of=OROS.img bs=1M count=64
	mkfs.fat -F 32 OROS.img
	sudo mkdir -p SYS/
	sudo mount -o loop OROS.img SYS/
	sudo mkdir -p SYS/EFI/BOOT/
	sudo cp bootloader_uefi/obj/BOOTX64.EFI SYS/EFI/BOOT/
	sudo umount SYS
	sudo rm -rf SYS/

	sudo qemu-system-x86_64 -enable-kvm -cpu host -smp 2 -m 2G -bios /usr/share/ovmf/OVMF.fd -cdrom OROS.img -boot d

clean:
	$(MAKE) -C bootloader_uefi clean
	rm OROS.img