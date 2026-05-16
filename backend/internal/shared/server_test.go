package shared

import (
	"log/slog"
	"net/http"
	"os"
	"testing"
	"time"
)

func TestNewServer_DefaultPort(t *testing.T) {
	logger := slog.New(slog.NewTextHandler(os.Stdout, nil))
	s := NewServer(logger, nil)

	if s.port != 8080 {
		t.Errorf("expected default port 8080, got %d", s.port)
	}
}

func TestNewServer_CustomPort(t *testing.T) {
	logger := slog.New(slog.NewTextHandler(os.Stdout, nil))
	s := NewServer(logger, nil, WithPort(9090))

	if s.port != 9090 {
		t.Errorf("expected port 9090, got %d", s.port)
	}
}

func TestServer_StartAndShutdown(t *testing.T) {
	logger := slog.New(slog.NewTextHandler(os.Stdout, &slog.HandlerOptions{Level: slog.LevelError}))

	handlers := []HandlerRegistration{
		{
			Pattern: "GET /health",
			Handler: func(w http.ResponseWriter, r *http.Request) {
				w.WriteHeader(http.StatusOK)
			},
		},
	}

	s := NewServer(logger, handlers, WithPort(0))
	// Use port 0 won't work with our implementation since we format the addr.
	// Instead use a high port unlikely to conflict.
	s = NewServer(logger, handlers, WithPort(18923))

	errCh := make(chan error, 1)
	go func() {
		errCh <- s.Start()
	}()

	// Give the server a moment to start listening.
	time.Sleep(50 * time.Millisecond)

	// Verify the server is responding.
	resp, err := http.Get("http://localhost:18923/health")
	if err != nil {
		t.Fatalf("failed to reach server: %v", err)
	}
	resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		t.Errorf("expected status 200, got %d", resp.StatusCode)
	}

	// Shutdown the server gracefully.
	if err := s.Shutdown(); err != nil {
		t.Fatalf("shutdown failed: %v", err)
	}

	// Start should have returned nil (ErrServerClosed is swallowed).
	if err := <-errCh; err != nil {
		t.Errorf("expected nil error from Start after shutdown, got: %v", err)
	}
}

func TestDrainTimeout_IsCorrect(t *testing.T) {
	if drainTimeout != 15*time.Second {
		t.Errorf("expected drain timeout of 15s, got %v", drainTimeout)
	}
}
