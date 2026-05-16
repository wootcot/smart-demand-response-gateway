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
	ws "github.com/smart-demand-response-gateway/backend/internal/features/websocket"
	"github.com/smart-demand-response-gateway/backend/internal/shared"
)

func main() {
	logger := setupLogger()

	handlers := wireHandlers(logger)

	srv := shared.NewServer(logger, handlers)

	go startServer(srv, logger)

	waitForShutdownSignal(logger)

	gracefulShutdown(srv, logger)
}

// setupLogger configures a structured JSON logger for production observability.
func setupLogger() *slog.Logger {
	logger := slog.New(slog.NewJSONHandler(os.Stdout, &slog.HandlerOptions{
		Level: slog.LevelInfo,
	}))
	slog.SetDefault(logger)
	return logger
}

// wireHandlers instantiates repositories, creates feature handlers with injected
// dependencies, and returns the composed handler registrations for the server router.
func wireHandlers(logger *slog.Logger) []shared.HandlerRegistration {
	telemetryRepo := telemetry.NewMemoryStore()
	gatewayRepo := gateway.NewMemoryStore()

	telemetryHandler := telemetry.NewHandler(telemetryRepo, logger)
	gatewayHandler := gateway.NewHandler(gatewayRepo)

	wsHub := ws.NewHub(logger)
	wsHandler := ws.NewHandler(wsHub, telemetryRepo, logger)

	return []shared.HandlerRegistration{
		{Pattern: "POST /telemetry", Handler: telemetryHandler.HandlePostTelemetry},
		{Pattern: "GET /status", Handler: gatewayHandler.HandleGetStatus},
		{Pattern: "GET /ws", Handler: wsHandler.HandleWebSocket},
	}
}

// startServer begins listening for connections. Exits the process on fatal errors.
func startServer(srv *shared.Server, logger *slog.Logger) {
	if err := srv.Start(); err != nil {
		logger.Error("server failed to start", slog.String("error", err.Error()))
		os.Exit(1)
	}
}

// waitForShutdownSignal blocks until SIGINT or SIGTERM is received.
func waitForShutdownSignal(logger *slog.Logger) {
	quit := make(chan os.Signal, 1)
	signal.Notify(quit, os.Interrupt, syscall.SIGTERM)
	sig := <-quit
	logger.Info("received shutdown signal", slog.String("signal", sig.String()))
}

// gracefulShutdown drains active connections within the configured timeout.
func gracefulShutdown(srv *shared.Server, logger *slog.Logger) {
	if err := srv.Shutdown(); err != nil {
		logger.Error("graceful shutdown failed", slog.String("error", err.Error()))
		os.Exit(1)
	}
	logger.Info("server exited cleanly")
}
