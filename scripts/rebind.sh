#!/bin/sh
#
# Opposite of unbind
#
set -e

DEV=0000:00:03.0
DRIVER_DIR=/sys/bus/pci/drivers/e1000e

if [ ! -d "$DRIVER_DIR" ]; then
    echo "e1000e driver not loaded"
    exit 1
fi

echo "$DEV" > "$DRIVER_DIR/bind"
echo "Bound $DEV to e1000e"

