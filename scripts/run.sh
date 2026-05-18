#!/bin/bash

cd "$(dirname "$0")/../vm"

qemu-system-x86_64 \
    -enable-kvm \
    -m 2G \
    -smp 2 \
    -drive file=ubuntu.qcow2,if=virtio \
    -device e1000e,netdev=net0,mac=52:54:00:12:34:56 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -nographic \
    #-display GTK \
    -s

