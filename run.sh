#!/bin/bash

cp ./x86_64/main.efi ./x86_64/EFI/boot/drivers_x64/keylogger.efi

qemu-system-x86_64 \
-drive file=dev.qcow2,format=qcow2 \
-bios /usr/share/OVMF/OVMF_CODE.fd \
-net none \
-drive file=fat:rw:x86_64/,format=raw \
-m 512M
