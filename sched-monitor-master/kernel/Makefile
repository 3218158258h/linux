# Makefile - Kernel module build
#
# Linux Kernel Performance Monitoring Platform
#
# Copyright (C) 2024

# Kernel build directory
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Module name
MODULE_NAME := sched_monitor

# Default target
all: module

# Build kernel module
module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Clean build artifacts
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f Module.symvers modules.order

# Install module
install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

# Load module
load:
	insmod $(MODULE_NAME).ko

# Unload module
unload:
	rmmod $(MODULE_NAME)

# Show module info
info:
	modinfo $(MODULE_NAME).ko

# Debug: show module parameters
debug:
	cat /sys/module/$(MODULE_NAME)/parameters/*

# Help
help:
	@echo "Available targets:"
	@echo "  all     - Build the kernel module (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  install - Install the module to system"
	@echo "  load    - Load the module"
	@echo "  unload  - Unload the module"
	@echo "  info    - Show module information"
	@echo "  debug   - Show module parameters"
	@echo ""
	@echo "Module parameters:"
	@echo "  buffer_size=N   - Per-CPU buffer size (default: 4096)"
	@echo "  interval_ms=N   - Workqueue interval in ms (default: 10)"
	@echo "  threshold_ns=N  - Anomaly threshold in ns (default: 100000000)"
	@echo "  filter_pid=N    - PID filter, 0 for no filter (default: 0)"
	@echo "  enabled=Y/N     - Enable on load (default: Y)"

.PHONY: all module clean install load unload info debug help
