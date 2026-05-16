# Backend (Go) commands

BACKEND_DIR := backend

.PHONY: be-build be-run be-test be-lint be-clean be-tidy

be-build: ## Compile the backend server
	cd $(BACKEND_DIR) && go build ./...

be-run: ## Run the backend server locally
	cd $(BACKEND_DIR) && go run ./cmd/api

be-test: ## Run all backend tests
	cd $(BACKEND_DIR) && go test ./... -v

be-lint: ## Run go vet on backend code
	cd $(BACKEND_DIR) && go vet ./...

be-tidy: ## Tidy go module dependencies
	cd $(BACKEND_DIR) && go mod tidy

be-clean: ## Remove backend build cache
	cd $(BACKEND_DIR) && go clean -cache
