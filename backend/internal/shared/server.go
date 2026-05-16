// Package shared provides common types and contracts used across feature packages.
package shared

import (
	"context"
	"errors"
	"fmt"
	"log/slog"
	"net/http"
	"time"
)

// defaultPort is the TCP port the server listens on when no custom port is configured.
const defaultPort = 8080

// drainTimeout is the maximum duration the server waits for active connections to complete
// during graceful shutdown. Connections still active after this period are forcefully closed.
const drainTimeout = 15 * time.Second

// HandlerRegistration represents a feature handler that can be mounted on the server's router.
// Each feature package exposes its HTTP handlers through this interface, allowing the composition
// root to wire features without the server package knowing about feature internals.
type HandlerRegistration struct {
	// Pattern is the HTTP route pattern (e.g., "POST /telemetry", "GET /status").
	Pattern string
	// Handler is the HTTP handler function for this route.
	Handler http.HandlerFunc
}

// Server encapsulates the HTTP server, its configuration, and lifecycle management.
// It composes feature handlers with shared middleware and provides graceful shutdown
// using context cancellation and a 15-second drain timeout.
type Server struct {
	httpServer *http.Server
	logger     *slog.Logger
	port       int
}

// ServerOption is a functional option for configuring the Server.
type ServerOption func(*Server)

// WithPort sets a custom TCP port for the server to listen on.
// If not specified, the server defaults to port 8080.
func WithPort(port int) ServerOption {
	return func(s *Server) {
		s.port = port
	}
}

// NewServer creates a new Server instance with the provided logger, handler registrations,
// and optional configuration. It composes the router by mounting all feature handlers and
// wrapping them with shared middleware (recovery, logging, CORS) in the correct order.
func NewServer(logger *slog.Logger, handlers []HandlerRegistration, opts ...ServerOption) *Server {
	s := &Server{
		logger: logger,
		port:   defaultPort,
	}

	for _, opt := range opts {
		opt(s)
	}

	mux := http.NewServeMux()
	for _, h := range handlers {
		mux.HandleFunc(h.Pattern, h.Handler)
	}

	// Middleware is applied in reverse order: the outermost middleware executes first.
	// Order: Recovery (outermost) -> Logging -> CORS -> Handler (innermost)
	// Recovery wraps everything so panics in any layer are caught.
	var handler http.Handler = mux
	handler = CORSMiddleware(handler)
	handler = LoggingMiddleware(logger)(handler)
	handler = RecoveryMiddleware(logger)(handler)

	s.httpServer = &http.Server{
		Addr:    fmt.Sprintf(":%d", s.port),
		Handler: handler,
	}

	return s
}

// Start begins listening for HTTP connections. It blocks until the server encounters
// a fatal error or is shut down via the Shutdown method. ErrServerClosed is not treated
// as an error since it indicates a normal graceful shutdown sequence.
func (s *Server) Start() error {
	s.logger.Info("server starting", slog.Int("port", s.port))

	err := s.httpServer.ListenAndServe()
	if err != nil && !errors.Is(err, http.ErrServerClosed) {
		return fmt.Errorf("server listen error: %w", err)
	}

	return nil
}

// Shutdown initiates graceful shutdown of the server. It creates a context with the
// 15-second drain timeout and asks the HTTP server to stop accepting new connections
// while allowing in-flight requests to complete. If connections do not drain within
// the timeout, they are forcefully closed and a non-nil error is returned.
func (s *Server) Shutdown() error {
	s.logger.Info("server shutting down", slog.Duration("drain_timeout", drainTimeout))

	ctx, cancel := context.WithTimeout(context.Background(), drainTimeout)
	defer cancel()

	if err := s.httpServer.Shutdown(ctx); err != nil {
		// Context deadline exceeded means connections did not drain in time.
		s.logger.Error("graceful shutdown exceeded drain timeout, forcing close",
			slog.String("error", err.Error()),
		)
		return fmt.Errorf("shutdown drain timeout exceeded: %w", err)
	}

	s.logger.Info("server stopped gracefully")
	return nil
}
