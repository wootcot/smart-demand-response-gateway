# Makefile for Smart Demand Response Gateway

.DEFAULT_GOAL := help

PORT ?= /dev/tty.usbserial-0001

.PHONY: help all-build all-test all-clean

help: ## Show this help message
	@echo ""
	@echo "Available targets:"
	@echo ""
	@echo "Global:"
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' Makefile | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "App (Flutter) — prefix: app-"
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' app/Makefile | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36mapp-%-21s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "Backend (Go) — prefix: be-"
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' backend/Makefile | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36mbe-%-22s\033[0m %s\n", $$1, $$2}'
	@echo ""
	@echo "Firmware (ESP32) — prefix: fw-"
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' firmware/Makefile | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36mfw-%-22s\033[0m %s\n", $$1, $$2}'
	@echo ""

# === Global targets ===

all-build: be-build app-analyze fw-build ## Build all components
all-test: be-test app-test ## Run all tests (backend + app)
all-clean: be-clean app-clean fw-clean ## Clean all build artifacts

# === App (Flutter) targets ===

app-%:
	$(MAKE) -C app $*

# === Backend (Go) targets ===

be-%:
	$(MAKE) -C backend $*

# === Firmware (ESP32) targets ===

fw-%:
	$(MAKE) -C firmware $* PORT=$(PORT)
