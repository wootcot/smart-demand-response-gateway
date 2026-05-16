// Package websocket implements the full-duplex WebSocket communication layer.
package websocket

import (
	"context"
	"encoding/json"
	"log/slog"
	"net/http"
	"time"

	"github.com/coder/websocket"

	"github.com/smart-demand-response-gateway/backend/internal/features/telemetry"
)

// Handler serves the WebSocket upgrade endpoint and manages gateway connections.
// Each connected gateway maintains a persistent full-duplex channel for telemetry
// ingestion and peak-stress command delivery with sub-second latency.
type Handler struct {
	hub           *Hub
	telemetryRepo telemetry.TelemetryRepository
	logger        *slog.Logger
	peakStress    bool
}

// NewHandler creates a WebSocket handler wired to the hub and telemetry repository.
func NewHandler(hub *Hub, telemetryRepo telemetry.TelemetryRepository, logger *slog.Logger) *Handler {
	return &Handler{
		hub:           hub,
		telemetryRepo: telemetryRepo,
		logger:        logger,
	}
}

// HandleWebSocket upgrades HTTP connections to WebSocket and manages the gateway
// communication lifecycle. The gateway identifies itself via the gateway_id query
// parameter on connection. Once connected, the server reads telemetry frames and
// can push peak-stress commands at any time without waiting for a request.
func (h *Handler) HandleWebSocket(w http.ResponseWriter, r *http.Request) {
	// Accept the WebSocket upgrade with permissive origin for development.
	conn, err := websocket.Accept(w, r, &websocket.AcceptOptions{
		InsecureSkipVerify: true,
	})
	if err != nil {
		h.logger.Error("websocket upgrade failed", slog.String("error", err.Error()))
		return
	}

	// Extract gateway ID from query parameter. The ESP32 firmware includes this
	// in the WebSocket URL: ws://host:port/ws?gateway_id=esp32-gw-001
	gatewayID := r.URL.Query().Get("gateway_id")
	if gatewayID == "" {
		gatewayID = "unknown"
	}

	client := NewClient(gatewayID, conn)
	h.hub.Register(client)

	defer func() {
		h.hub.Unregister(client)
		client.Close()
	}()

	h.logger.Info("gateway websocket session started", slog.String("gateway_id", gatewayID))

	// Read loop: process incoming telemetry frames from the gateway.
	// The connection stays open indefinitely until the gateway disconnects
	// or the server shuts down.
	ctx := r.Context()
	for {
		data, err := client.ReadMessage(ctx)
		if err != nil {
			// Connection closed by gateway or network error — exit read loop.
			h.logger.Info("gateway connection closed",
				slog.String("gateway_id", gatewayID),
				slog.String("reason", err.Error()),
			)
			return
		}

		h.handleIncomingMessage(ctx, client, data)
	}
}

// handleIncomingMessage routes and processes a single frame from a gateway.
func (h *Handler) handleIncomingMessage(ctx context.Context, client *Client, data []byte) {
	var msg TelemetryMessage
	if err := json.Unmarshal(data, &msg); err != nil {
		h.logger.Warn("invalid websocket message",
			slog.String("gateway_id", client.GatewayID),
			slog.String("error", err.Error()),
		)
		return
	}

	// Store the telemetry reading in the repository.
	reading := telemetry.TelemetryReading{
		GatewayID:  client.GatewayID,
		PowerWatts: msg.PowerWatts,
		Timestamp:  time.Now().UTC(),
	}

	if err := h.telemetryRepo.Store(ctx, reading); err != nil {
		h.logger.Error("failed to store websocket telemetry",
			slog.String("gateway_id", client.GatewayID),
			slog.String("error", err.Error()),
		)
		return
	}

	// Send acknowledgment with current peak-stress state back to gateway.
	// This provides immediate feedback on every telemetry frame, giving the
	// gateway sub-second awareness of peak-stress state changes.
	ack := AckMessage{
		Type:             MsgTypeAck,
		Acknowledged:     true,
		PeakStressActive: h.peakStress,
	}

	ackData, _ := json.Marshal(ack)
	if err := client.conn.Write(ctx, websocket.MessageText, ackData); err != nil {
		h.logger.Warn("failed to send ack",
			slog.String("gateway_id", client.GatewayID),
			slog.String("error", err.Error()),
		)
	}

	h.logger.Debug("telemetry received via websocket",
		slog.String("gateway_id", client.GatewayID),
		slog.Float64("power_watts", msg.PowerWatts),
	)
}

// SetPeakStress updates the peak stress state and broadcasts to all connected gateways.
func (h *Handler) SetPeakStress(ctx context.Context, active bool) {
	h.peakStress = active
	h.hub.SetPeakStress(ctx, active)
}
