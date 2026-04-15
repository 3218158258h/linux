# Makefile - Top-level build script
#
# Linux Kernel Performance Monitoring Platform
#
# Copyright (C) 2024

.PHONY: all kernel user clean install help

# Default target
all: kernel user

# ========== Kernel Module ==========
kernel:
	$(MAKE) -C kernel

kernel-clean:
	$(MAKE) -C kernel clean

kernel-install:
	$(MAKE) -C kernel install

kernel-load:
	$(MAKE) -C kernel load

kernel-unload:
	$(MAKE) -C kernel unload

# ========== User-space Tools ==========
user:
	mkdir -p build && cd build && cmake .. && $(MAKE)

user-clean:
	rm -rf build

user-install:
	cd build && $(MAKE) install

# ========== Combined Targets ==========
clean: kernel-clean user-clean

install: kernel-install user-install

# ========== Development Helpers ==========
test: all
	@echo "Loading kernel module..."
	-sudo insmod kernel/sched_monitor.ko
	@echo "Starting daemon..."
	-build/monitord -l /tmp/monitord.log -p /tmp/monitord.pid &
	@echo "Running schedtop..."
	-build/schedtop

stop:
	-killall schedtop
	-killall monitord
	-sudo rmmod sched_monitor

# ========== Help ==========
help:
	@echo "Linux Kernel Performance Monitoring Platform"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all            Build kernel module and user tools (default)"
	@echo "  kernel         Build kernel module only"
	@echo "  user           Build user tools only"
	@echo "  clean          Remove all build artifacts"
	@echo "  install        Install to system"
	@echo ""
	@echo "Development:"
	@echo "  test           Build, load module, start daemon, run schedtop"
	@echo "  stop           Stop all components"
	@echo ""
	@echo "Kernel module:"
	@echo "  kernel-load    Load kernel module"
	@echo "  kernel-unload  Unload kernel module"
	@echo ""
	@echo "User tools:"
	@echo "  user-install   Install user tools to /usr/local/bin"
	@echo ""
	@echo "Module parameters (load with insmod):"
	@echo "  buffer_size=N   Per-CPU buffer size (default: 4096)"
	@echo "  interval_ms=N   Workqueue interval (default: 10)"
	@echo "  threshold_ns=N  Anomaly threshold (default: 100000000)"
	@echo "  filter_pid=N    PID filter (default: 0 = no filter)"
	@echo "  enabled=Y/N     Enable on load (default: Y)"
