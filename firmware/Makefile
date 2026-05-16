# Firmware (ESP32 / ESP-IDF) commands
# Requires ESP-IDF toolchain sourced: source $IDF_PATH/export.sh

PORT ?= /dev/tty.usbserial-0001

.DEFAULT_GOAL := help

.PHONY: help build flash monitor menuconfig clean fullclean flash-monitor

help: ## Show this help message
	@echo ""
	@echo "Firmware (ESP32) targets:"
	@echo ""
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2}'
	@echo ""

build: ## Build firmware using idf.py
	idf.py build

flash: ## Flash firmware to ESP32 (PORT=/dev/ttyUSB0)
	idf.py -p $(PORT) flash

monitor: ## Open serial monitor for ESP32 (PORT=/dev/ttyUSB0)
	idf.py -p $(PORT) monitor

menuconfig: ## Open ESP-IDF menuconfig (Wi-Fi, partitions, etc.)
	idf.py menuconfig

clean: ## Clean firmware build artifacts
	idf.py fullclean

flash-monitor: ## Flash and immediately open monitor
	idf.py -p $(PORT) flash monitor
