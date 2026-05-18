// Package websocket implements the full-duplex WebSocket communication layer.
package websocket

import (
	"context"
	"encoding/json"
	"log/slog"
	"sync"

	"github.com/coder/websocket"
	"github.com/google/uuid"
)

// DashboardClient represents a connected Flutter dashboard over WebSocket.
// Unlike gateway clients, dashboard clients only receive status pushes.
type DashboardClient struct {
	ID   string
	conn *websocket.Conn
}

// NewDashboardClient wraps a WebSocket connection for a dashboard viewer.
func NewDashboardClient(conn *websocket.Conn) *DashboardClient {
	return &DashboardClient{
		ID:   uuid.NewString(),
		conn: conn,
	}
}

// Close gracefully closes the dashboard WebSocket connection.
func (c *DashboardClient) Close() error {
	return c.conn.Close(websocket.StatusNormalClosure, "closing")
}

// DashboardHub manages all connected dashboard WebSocket clients and provides
// broadcast capabilities for pushing grid status updates in real time.
type DashboardHub struct {
	mu      sync.RWMutex
	clients map[string]*DashboardClient
	logger  *slog.Logger
}

// NewDashboardHub creates a DashboardHub ready to accept dashboard connections.
func NewDashboardHub(logger *slog.Logger) *DashboardHub {
	return &DashboardHub{
		clients: make(map[string]*DashboardClient),
		logger:  logger,
	}
}

// Register adds a dashboard client to the hub.
func (h *DashboardHub) Register(client *DashboardClient) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.clients[client.ID] = client
	h.logger.Info("dashboard client registered",
		slog.String("id", client.ID),
		slog.Int("total_dashboard_clients", len(h.clients)),
	)
}

// Unregister removes a dashboard client from the hub.
func (h *DashboardHub) Unregister(client *DashboardClient) {
	h.mu.Lock()
	defer h.mu.Unlock()
	delete(h.clients, client.ID)
	h.logger.Info("dashboard client unregistered",
		slog.String("id", client.ID),
		slog.Int("total_dashboard_clients", len(h.clients)),
	)
}

// Broadcast sends a DashboardStatusMessage to all connected dashboard clients.
func (h *DashboardHub) Broadcast(ctx context.Context, msg DashboardStatusMessage) {
	h.mu.RLock()
	defer h.mu.RUnlock()

	data, err := json.Marshal(msg)
	if err != nil {
		h.logger.Error("failed to marshal dashboard status", slog.String("error", err.Error()))
		return
	}

	for id, client := range h.clients {
		if err := client.conn.Write(ctx, websocket.MessageText, data); err != nil {
			h.logger.Warn("failed to send status to dashboard client",
				slog.String("id", id),
				slog.String("error", err.Error()),
			)
		}
	}
}
