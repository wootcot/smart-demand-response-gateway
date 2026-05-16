// Package gateway provides the gateway status feature for the demand-response system.
// It tracks the operational state of connected ESP32 gateways, including their last
// reported power readings and peak stress status. This package is self-contained with
// its own domain types, repository interface, and HTTP handler — it depends only on
// the shared package and the standard library.
package gateway

import "context"

// GatewayRepository defines the persistence contract for gateway status data.
// Implementations handle storage and retrieval of gateway operational states.
// The interface uses context.Context to support cancellation and deadline propagation,
// enabling graceful shutdown behavior to flow through the data layer.
type GatewayRepository interface {
	// UpdateStatus persists or updates the operational state of a gateway.
	// If a gateway with the same ID already exists, its record is overwritten
	// with the new status values.
	UpdateStatus(ctx context.Context, status GatewayStatus) error

	// GetAll retrieves the current status of all known gateways.
	// Returns an empty slice (not nil) when no gateways have reported.
	GetAll(ctx context.Context) ([]GatewayStatus, error)
}
