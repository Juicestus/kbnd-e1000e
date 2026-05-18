#!/bin/sh
# Unbind the e1000e kernel driver from PCI 0000:00:03.0
#
# Run inside guest as su

set -e

DEV=0000:00:03.0
DRIVER_DIR=/sys/bus/pci/drivers/e1000e

if [ ! -d "$DRIVER_DIR" ]; then
    echo "e1000e driver not loaded — nothing to unbind"
    exit 0
fi

if [ -e "$DRIVER_DIR/$DEV" ]; then
    echo "$DEV" > "$DRIVER_DIR/unbind"
    echo "Unbound $DEV from e1000e"
else
    echo "$DEV is not currently bound to e1000e"
fi

# Confirm
if [ -e "/sys/bus/pci/devices/$DEV/driver" ]; then
    DRIVER=$(basename $(readlink /sys/bus/pci/devices/$DEV/driver))
    echo "Device $DEV still has driver: $DRIVER"
else
    echo "Device $DEV is now driverless (good)"
fi
