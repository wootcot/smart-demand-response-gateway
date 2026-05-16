// Package gateway provides the gateway status feature for the demand-response system.
// It tracks the operational state of connected ESP32 gateways, including their last
// reported power readings and peak stress status. This package is self-contained with
// its own domain types, repository interface, and HTTP handler — it depends only on
// the shared package and the standard library.
package gateway

import (
	"context"
	"sync"
)

// MemoryStore implements GatewayRepository with in-memory storage protected by a
// read-write mutex. This implementation is suitable for single-instance deployments
// and development. It can be replaced with a PostgreSQL-backed implementation without
// changing the handler or domain code, thanks to the repository interface abstraction.
type MemoryStore struct {
	mu       sync.RWMutex
	gateways map[string]GatewayStatus
}

// NewMemoryStore creates a MemoryStore with an initialized internal map.
func NewMemoryStore() *MemoryStore {
	return &MemoryStore{
		gateways: make(map[string]GatewayStatus),
	}
}

// UpdateStatus persists or overwrites the status of a gateway identified by its ID.
// The write lock ensures concurrent telemetry ingestion from multiple gateways does
// not corrupt the map state.
func (m *MemoryStore) UpdateStatus(_ context.Context, status GatewayStatus) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	m.gateways[status.GatewayID] = status
	return nil
}

// GetAll returns a snapshot of all known gateway statuses. The read lock allows
// concurrent dashboard polling without blocking telemetry writes for longer than
// necessary. Returns an empty slice when no gateways have reported.
func (m *MemoryStore) GetAll(_ context.Context) ([]GatewayStatus, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	statuses := make([]GatewayStatus, 0, len(m.gateways))
	for _, gs := range m.gateways {
		statuses = append(statuses, gs)
	}
	return statuses, nil
}
