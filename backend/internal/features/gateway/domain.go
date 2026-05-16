// Package gateway provides the gateway status feature for the demand-response system.
// It tracks the operational state of connected ESP32 gateways, including their last
// reported power readings and peak stress status. This package is self-contained with
// its own domain types, repository interface, and HTTP handler — it depends only on
// the shared package and the standard library.
package gateway

import "time"

// GatewayStatus represents the current operational state of a connected gateway device.
// Each gateway periodically reports its power reading via the telemetry endpoint, and
// this entity captures the most recent snapshot of that gateway's state for dashboard
// aggregation and grid load computation.
type GatewayStatus struct {
	GatewayID        string    `json:"gateway_id"`
	LastSeen         time.Time `json:"last_seen"`
	LastPowerWatts   float64   `json:"last_power_watts"`
	PeakStressActive bool      `json:"peak_stress_active"`
}
