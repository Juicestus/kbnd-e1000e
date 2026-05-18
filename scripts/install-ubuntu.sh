#!/bin/bash
cd "$(dirname "$0")/../vm"

ISO=$(ls ubuntu-*-live-server-amd64.iso | head -n1)

qemu-system-x86_64 \
    -enable-kvm \
    -m 2G \
    -smp 2 \
    -cdrom "$ISO" \
    -drive file=ubuntu.qcow2,if=virtio \
    -boot d \
    -device e1000e,netdev=net0,mac=52:54:00:12:34:56 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    #-nographic
    -display gtk
