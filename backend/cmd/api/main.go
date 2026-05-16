// Package main is the composition root for the Nepal Grid Peak Load Controller backend.
// It wires together feature repositories, handlers, shared middleware, and the HTTP server,
// then manages the process lifecycle via OS signal handling and graceful shutdown.
package main

import (
	"log/slog"
	"os"
	"os/signal"
	"syscall"

	"github.com/smart-demand-response-gateway/backend/internal/features/gateway"
	"github.com/smart-demand-response-gateway/backend/internal/features/telemetry"
	"github.com/smart-demand-response-gateway/backend/internal/shared"
)

func main() {
	// Configure structured JSON logger for production observability.
	logger := slog.New(slog.NewJSONHandler(os.Stdout, &slog.HandlerOptions{
		Level: slog.LevelInfo,
	}))
	slog.SetDefault(logger)

	// Instantiate in-memory repositories for each feature.
	// These can be swapped for database-backed implementations without changing handlers.
	telemetryRepo := telemetry.NewMemoryStore()
	gatewayRepo := gateway.NewMemoryStore()

	// Create feature handlers with injected repository dependencies.
	telemetryHandler := telemetry.NewHandler(telemetryRepo, logger)
	gatewayHandler := gateway.NewHandler(gatewayRepo)

	// Compose handler registrations for the shared server router.
	handlers := []shared.HandlerRegistration{
		{Pattern: "POST /telemetry", Handler: telemetryHandler.HandlePostTelemetry},
		{Pattern: "GET /status", Handler: gatewayHandler.HandleGetStatus},
	}

	// Create the HTTP server with default port (8080) and composed middleware stack.
	srv := shared.NewServer(logger, handlers)

	// Start the server in a background goroutine so the main goroutine can
	// block on OS signals for lifecycle management.
	go func() {
		if err := srv.Start(); err != nil {
			logger.Error("server failed to start", slog.String("error", err.Error()))
			os.Exit(1)
		}
	}()

	// Block until a termination signal is received. This allows the server to run
	// indefinitely until an operator or orchestrator requests shutdown.
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, os.Interrupt, syscall.SIGTERM)
	sig := <-quit
	logger.Info("received shutdown signal", slog.String("signal", sig.String()))

	// Initiate graceful shutdown: drain active connections within the 15-second timeout.
	if err := srv.Shutdown(); err != nil {
		logger.Error("graceful shutdown failed", slog.String("error", err.Error()))
		os.Exit(1)
	}

	logger.Info("server exited cleanly")
}
