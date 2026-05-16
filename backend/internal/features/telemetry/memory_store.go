// Package telemetry implements the telemetry ingestion feature for the demand-response gateway.
// It receives power readings from ESP32 gateways and stores them for aggregation.
// This package depends only on the shared package and standard library, maintaining
// feature isolation per the Clean Architecture dependency rule.
package telemetry

import (
	"context"
	"sync"
)

// maxReadingsPerGateway limits stored readings per gateway to prevent unbounded
// memory growth in the in-memory implementation. Production deployments should
// use a database-backed repository with proper retention policies.
const maxReadingsPerGateway = 100

// MemoryStore implements TelemetryRepository with in-memory storage.
// It uses a sync.RWMutex to allow concurrent reads while serializing writes,
// which matches the expected access pattern: frequent reads from the dashboard
// and periodic writes from gateways on 10-second intervals.
type MemoryStore struct {
	mu       sync.RWMutex
	readings map[string][]TelemetryReading
}

// NewMemoryStore creates a ready-to-use in-memory telemetry store.
func NewMemoryStore() *MemoryStore {
	return &MemoryStore{
		readings: make(map[string][]TelemetryReading),
	}
}

// Store persists a telemetry reading in memory, keyed by gateway ID.
// Older readings are evicted when the per-gateway limit is reached.
func (m *MemoryStore) Store(_ context.Context, reading TelemetryReading) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	entries := m.readings[reading.GatewayID]
	entries = append(entries, reading)

	// Evict oldest readings when capacity is exceeded to bound memory usage.
	if len(entries) > maxReadingsPerGateway {
		entries = entries[len(entries)-maxReadingsPerGateway:]
	}

	m.readings[reading.GatewayID] = entries
	return nil
}

// GetLatest returns all stored readings for a gateway, ordered by insertion
// (oldest first). Returns an empty slice if no readings exist.
func (m *MemoryStore) GetLatest(_ context.Context, gatewayID string) ([]TelemetryReading, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	entries := m.readings[gatewayID]
	if entries == nil {
		return []TelemetryReading{}, nil
	}

	// Return a copy to prevent callers from mutating internal state.
	result := make([]TelemetryReading, len(entries))
	copy(result, entries)
	return result, nil
}
