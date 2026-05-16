// Package shared provides common types and contracts used across feature packages.
package shared

import "context"

// HealthChecker defines a contract for components that can report their health status.
// Feature packages implement this interface to participate in system-wide health checks
// without introducing cross-feature dependencies.
type HealthChecker interface {
	// Healthy returns nil if the component is operating normally, or an error
	// describing the degraded condition.
	Healthy(ctx context.Context) error
}
