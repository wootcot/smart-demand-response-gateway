# Backend (Go) commands

.DEFAULT_GOAL := help

.PHONY: help build run test lint clean tidy

help: ## Show this help message
	@echo ""
	@echo "Backend (Go) targets:"
	@echo ""
	@grep -E '^[a-zA-Z0-9_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-25s\033[0m %s\n", $$1, $$2}'
	@echo ""

build: ## Compile the backend server
	go build ./...

run: ## Run the backend server locally
	go run ./cmd/api

test: ## Run all backend tests
	go test ./... -v

lint: ## Run go vet on backend code
	go vet ./...

tidy: ## Tidy go module dependencies
	go mod tidy

clean: ## Remove backend build cache
	go clean -cache
