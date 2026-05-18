#!/bin/bash

cd "$(dirname "$0")/../vm"

qemu-system-x86_64 \
    -enable-kvm \
    -m 2G \
    -smp 2 \
    -drive file=ubuntu.qcow2,if=virtio \
    -device e1000e,netdev=target,mac=52:54:00:12:34:56 \
    -netdev user,id=target,net=10.0.3.0/24,host=10.0.3.2,dhcpstart=10.0.3.15 \
    -device virtio-net-pci,netdev=mgmt,mac=52:54:00:aa:bb:cc \
    -netdev user,id=mgmt,net=10.0.4.0/24,host=10.0.4.2,dhcpstart=10.0.4.15,hostfwd=tcp::2222-:22 \
    -nographic \
    -s

    #-device e1000e,netdev=net0,mac=52:54:00:12:34:56 \
    #-netdev user,id=net0,hostfwd=tcp::2222-:22 \
    #-display GTK \
    #
