#!/bin/bash
#
# test.sh - Test script for sched_monitor
#
# Linux Kernel Performance Monitoring Platform
#
# Copyright (C) 2024

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Test functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root for kernel module operations"
        exit 1
    fi
}

# Check kernel headers
check_kernel_headers() {
    local kernel_version=$(uname -r)
    local headers_path="/lib/modules/${kernel_version}/build"
    
    if [[ ! -d "$headers_path" ]]; then
        log_error "Kernel headers not found at $headers_path"
        log_info "Install with: sudo apt install linux-headers-$(uname -r)"
        exit 1
    fi
    
    log_info "Kernel headers found: $headers_path"
}

# Build kernel module
build_kernel() {
    log_info "Building kernel module..."
    cd "$PROJECT_ROOT/kernel"
    make clean
    make
    log_info "Kernel module built successfully"
}

# Build user tools
build_user() {
    log_info "Building user tools..."
    cd "$PROJECT_ROOT"
    rm -rf build
    mkdir -p build
    cd build
    cmake ..
    make
    log_info "User tools built successfully"
}

# Load kernel module
load_module() {
    log_info "Loading kernel module..."
    
    # Unload if already loaded
    if lsmod | grep -q sched_monitor; then
        rmmod sched_monitor || true
    fi
    
    insmod "$PROJECT_ROOT/kernel/sched_monitor.ko" \
        buffer_size=4096 \
        interval_ms=10 \
        threshold_ns=100000000 \
        enabled=1
    
    if lsmod | grep -q sched_monitor; then
        log_info "Kernel module loaded successfully"
    else
        log_error "Failed to load kernel module"
        exit 1
    fi
}

# Unload kernel module
unload_module() {
    log_info "Unloading kernel module..."
    
    if lsmod | grep -q sched_monitor; then
        rmmod sched_monitor
        log_info "Kernel module unloaded"
    else
        log_warn "Kernel module not loaded"
    fi
}

# Test Netlink communication
test_netlink() {
    log_info "Testing Netlink communication..."
    
    # Run a quick test with schedtop
    timeout 5 "$PROJECT_ROOT/build/schedtop" || true
    
    log_info "Netlink test completed"
}

# Run stress test
stress_test() {
    log_info "Running stress test..."
    
    # Start stress-ng to generate scheduling events
    if command -v stress-ng &> /dev/null; then
        stress-ng --cpu 4 --timeout 10s &
        sleep 2
        
        # Monitor for a few seconds
        timeout 8 "$PROJECT_ROOT/build/schedtop" || true
        
        wait
    else
        log_warn "stress-ng not installed, skipping stress test"
        log_info "Install with: sudo apt install stress-ng"
    fi
    
    log_info "Stress test completed"
}

# Check kernel log
check_dmesg() {
    log_info "Checking kernel log..."
    
    dmesg | tail -20 | grep -i sched_monitor || true
}

# Main test sequence
main() {
    log_info "=========================================="
    log_info "sched_monitor Test Script"
    log_info "=========================================="
    
    # Build
    build_kernel
    build_user
    
    # Check root for module operations
    check_root
    
    # Load and test
    load_module
    check_dmesg
    
    # Run tests
    test_netlink
    stress_test
    
    # Cleanup
    unload_module
    
    log_info "=========================================="
    log_info "All tests completed!"
    log_info "=========================================="
}

# Handle command line arguments
case "${1:-all}" in
    build)
        build_kernel
        build_user
        ;;
    load)
        check_root
        load_module
        ;;
    unload)
        check_root
        unload_module
        ;;
    test)
        test_netlink
        ;;
    stress)
        stress_test
        ;;
    all)
        main
        ;;
    *)
        echo "Usage: $0 {build|load|unload|test|stress|all}"
        exit 1
        ;;
esac
