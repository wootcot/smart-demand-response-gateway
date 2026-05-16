# Makefile for Smart Demand Response Gateway

# Include command modules
include makefiles/*.mk

.DEFAULT_GOAL := help

.PHONY: help all-build all-test all-clean

help: ## Show this help message
	@echo ""
	@echo "Available targets:"
	@echo ""
	@for mkFile in Makefile makefiles/*.mk; do \
		if [ -f "$$mkFile" ]; then \
			echo "From $$mkFile:"; \
			grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' "$$mkFile" | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2}'; \
			echo ""; \
		fi \
	done

all-build: be-build app-analyze fw-build ## Build all components
all-test: be-test app-test ## Run all tests (backend + app)
all-clean: be-clean app-clean fw-clean ## Clean all build artifacts
