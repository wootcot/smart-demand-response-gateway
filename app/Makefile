# App (Flutter) commands

.DEFAULT_GOAL := help

.PHONY: help analyze run test build-runner build-apk clean get

help: ## Show this help message
	@echo ""
	@echo "App (Flutter) targets:"
	@echo ""
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2}'
	@echo ""

get: ## Fetch Flutter dependencies
	flutter pub get

analyze: ## Run Flutter static analysis
	flutter analyze

run: ## Run the Flutter app in debug mode
	flutter run

test: ## Run Flutter tests
	flutter test

build-runner: ## Run code generation (Riverpod providers)
	dart run build_runner build --delete-conflicting-outputs

build-apk: ## Build release APK
	flutter build apk --release

clean: ## Clean Flutter build artifacts
	flutter clean
