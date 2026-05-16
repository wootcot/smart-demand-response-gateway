# App (Flutter) commands

APP_DIR := app

.PHONY: app-analyze app-run app-test app-build-runner app-clean app-get app-build-apk

app-get: ## Fetch Flutter dependencies
	cd $(APP_DIR) && flutter pub get

app-analyze: ## Run Flutter static analysis
	cd $(APP_DIR) && flutter analyze

app-run: ## Run the Flutter app in debug mode
	cd $(APP_DIR) && flutter run

app-test: ## Run Flutter tests
	cd $(APP_DIR) && flutter test

app-build-runner: ## Run code generation (Riverpod providers)
	cd $(APP_DIR) && dart run build_runner build --delete-conflicting-outputs

app-build-apk: ## Build release APK
	cd $(APP_DIR) && flutter build apk --release

app-clean: ## Clean Flutter build artifacts
	cd $(APP_DIR) && flutter clean
