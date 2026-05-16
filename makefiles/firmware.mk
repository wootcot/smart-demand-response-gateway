# Firmware (ESP32 / ESP-IDF) commands
# Requires ESP-IDF toolchain sourced: source $IDF_PATH/export.sh

FIRMWARE_DIR := firmware
PORT ?= /dev/tty.usbserial-0001

.PHONY: fw-build fw-flash fw-monitor fw-menuconfig fw-clean fw-fullclean

fw-build: ## Build firmware using idf.py
	cd $(FIRMWARE_DIR) && idf.py build

fw-flash: ## Flash firmware to ESP32 (PORT=/dev/ttyUSB0)
	cd $(FIRMWARE_DIR) && idf.py -p $(PORT) flash

fw-monitor: ## Open serial monitor for ESP32 (PORT=/dev/ttyUSB0)
	cd $(FIRMWARE_DIR) && idf.py -p $(PORT) monitor

fw-menuconfig: ## Open ESP-IDF menuconfig (Wi-Fi, partitions, etc.)
	cd $(FIRMWARE_DIR) && idf.py menuconfig

fw-clean: ## Clean firmware build artifacts
	cd $(FIRMWARE_DIR) && idf.py fullclean

fw-flash-monitor: ## Flash and immediately open monitor
	cd $(FIRMWARE_DIR) && idf.py -p $(PORT) flash monitor
