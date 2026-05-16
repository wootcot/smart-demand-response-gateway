// Package websocket implements the full-duplex WebSocket communication layer
// between ESP32 gateways and the backend control server. It provides low-latency
// bidirectional messaging for telemetry ingestion and peak-stress command broadcast,
// replacing traditional HTTP polling with a persistent connection model.
package websocket

import (
	"context"
	"log/slog"
	"sync"
)

// Hub manages all active gateway WebSocket connections and provides broadcast
// capabilities for peak-stress instructions. It maintains a registry of connected
// gateways and fans out commands to all or specific gateways.
type Hub struct {
	mu      sync.RWMutex
	clients map[string]*Client
	logger  *slog.Logger
}

// NewHub creates a Hub ready to accept gateway connections.
func NewHub(logger *slog.Logger) *Hub {
	return &Hub{
		clients: make(map[string]*Client),
		logger:  logger,
	}
}

// Register adds a gateway client to the hub's active connection registry.
func (h *Hub) Register(client *Client) {
	h.mu.Lock()
	defer h.mu.Unlock()

	h.clients[client.GatewayID] = client
	h.logger.Info("gateway connected",
		slog.String("gateway_id", client.GatewayID),
		slog.Int("total_connections", len(h.clients)),
	)
}

// Unregister removes a gateway client from the hub when its connection closes.
func (h *Hub) Unregister(client *Client) {
	h.mu.Lock()
	defer h.mu.Unlock()

	delete(h.clients, client.GatewayID)
	h.logger.Info("gateway disconnected",
		slog.String("gateway_id", client.GatewayID),
		slog.Int("total_connections", len(h.clients)),
	)
}

// BroadcastPeakStress sends a peak-stress instruction to all connected gateways.
// This enables the backend to push load-shedding commands with minimal latency
// rather than waiting for gateways to poll on their next interval.
func (h *Hub) BroadcastPeakStress(ctx context.Context, active bool) {
	h.mu.RLock()
	defer h.mu.RUnlock()

	msg := PeakStressMessage{PeakStressActive: active}

	for id, client := range h.clients {
		if err := client.SendPeakStress(ctx, msg); err != nil {
			h.logger.Warn("failed to send peak stress to gateway",
				slog.String("gateway_id", id),
				slog.String("error", err.Error()),
			)
		}
	}

	h.logger.Info("peak stress broadcast complete",
		slog.Bool("active", active),
		slog.Int("recipients", len(h.clients)),
	)
}

// ConnectedCount returns the number of currently connected gateways.
func (h *Hub) ConnectedCount() int {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return len(h.clients)
}

// SetPeakStress updates peak stress state and broadcasts to all gateways.
func (h *Hub) SetPeakStress(ctx context.Context, active bool) {
	h.BroadcastPeakStress(ctx, active)
}
