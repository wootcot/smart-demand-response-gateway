// Package telemetry implements the telemetry ingestion feature for the demand-response gateway.
// It receives power readings from ESP32 gateways and stores them for aggregation.
// This package depends only on the shared package and standard library, maintaining
// feature isolation per the Clean Architecture dependency rule.
package telemetry

import "time"

// TelemetryReading represents a single power measurement from a gateway.
// Each reading captures the instantaneous smoothed power draw reported by
// the gateway's CT clamp sensor after its 3-second rolling average computation.
type TelemetryReading struct {
	GatewayID  string    `json:"gateway_id"`
	PowerWatts float64   `json:"power_watts"`
	Timestamp  time.Time `json:"timestamp"`
}
