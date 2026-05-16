// Package telemetry implements the telemetry ingestion feature for the demand-response gateway.
// It receives power readings from ESP32 gateways and stores them for aggregation.
// This package depends only on the shared package and standard library, maintaining
// feature isolation per the Clean Architecture dependency rule.
package telemetry

import "context"

// TelemetryRepository defines the persistence contract for telemetry readings.
// Implementations may use in-memory storage (for development) or PostgreSQL
// (for production) without requiring changes to the handler or domain logic.
// This interface lives within the feature package because it is specific to
// telemetry concerns and does not need cross-feature sharing.
type TelemetryRepository interface {
	// Store persists a single telemetry reading. Returns an error if the
	// write operation fails.
	Store(ctx context.Context, reading TelemetryReading) error

	// GetLatest retrieves the most recent telemetry readings for a given
	// gateway, ordered by timestamp descending. The slice may be empty if
	// no readings exist for the specified gateway.
	GetLatest(ctx context.Context, gatewayID string) ([]TelemetryReading, error)
}
