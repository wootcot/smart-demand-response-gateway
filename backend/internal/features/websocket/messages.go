// Package websocket implements the full-duplex WebSocket communication layer.
package websocket

import "github.com/smart-demand-response-gateway/backend/internal/features/gateway"

// TelemetryMessage is the JSON payload sent by gateways over WebSocket.
// Mirrors the HTTP TelemetryRequest but delivered over the persistent connection.
type TelemetryMessage struct {
	Type       string  `json:"type"`
	GatewayID  string  `json:"gateway_id"`
	PowerWatts float64 `json:"power_watts"`
}

// PeakStressMessage is the JSON command pushed from backend to gateways.
// Delivered instantly over the WebSocket without waiting for gateway polling.
type PeakStressMessage struct {
	Type             string `json:"type"`
	PeakStressActive bool   `json:"peak_stress_active"`
}

// AckMessage is sent back to the gateway confirming telemetry receipt.
type AckMessage struct {
	Type             string `json:"type"`
	Acknowledged     bool   `json:"acknowledged"`
	PeakStressActive bool   `json:"peak_stress_active"`
}

// DashboardStatusMessage is the JSON payload pushed to dashboard WebSocket clients.
// Contains the full grid status so the Flutter app can render without polling.
type DashboardStatusMessage struct {
	Type           string                  `json:"type"`
	Gateways       []gateway.GatewayStatus `json:"gateways"`
	TotalLoadWatts float64                 `json:"total_load_watts"`
	PeakActive     bool                    `json:"peak_active"`
}

// Message type constants for routing incoming/outgoing frames.
const (
	MsgTypeTelemetry       = "telemetry"
	MsgTypePeakStress      = "peak_stress"
	MsgTypeAck             = "ack"
	MsgTypeDashboardStatus = "dashboard_status"
)
