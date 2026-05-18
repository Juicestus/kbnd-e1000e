CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -g -static
LDFLAGS :=

BUILD_DIR := build
SRC_DIR   := src

BIN       := kbnd
TARGET    := $(BUILD_DIR)/$(BIN)
SOURCES   := $(wildcard $(SRC_DIR)/*.c)

GUEST_DIR := /home/kbnd/kbnd
SSH_HOST  := kbnd-vm

.PHONY: all ship run clean help

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS)
	@echo "Built $@"

$(BUILD_DIR):
	@mkdir -p $@

ship: $(TARGET)
	@ssh $(SSH_HOST) "mkdir -p $(GUEST_DIR)"
	@scp -q $(TARGET) $(SSH_HOST):$(GUEST_DIR)/
	@echo "Shipped $(TARGET) to $(SSH_HOST):$(GUEST_DIR)/"

run: ship
	@echo "Running $(BIN) on guest:\n\n\n"
	@ssh $(SSH_HOST) "$(GUEST_DIR)/$(BIN)"

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  make        Build the binary"
	@echo "  make ship   Build and scp to guest"
	@echo "  make run    Build, ship, and execute in guest"
	@echo "  make clean  Remove build artifacts"

