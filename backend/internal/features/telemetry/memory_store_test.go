package telemetry

import (
	"context"
	"testing"
	"time"
)

func TestMemoryStore_StoreAndGetLatest(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	reading := TelemetryReading{
		GatewayID:  "gw-001",
		PowerWatts: 1200.0,
		Timestamp:  time.Now().UTC(),
	}

	if err := store.Store(ctx, reading); err != nil {
		t.Fatalf("unexpected error storing reading: %v", err)
	}

	readings, err := store.GetLatest(ctx, "gw-001")
	if err != nil {
		t.Fatalf("unexpected error getting readings: %v", err)
	}

	if len(readings) != 1 {
		t.Fatalf("expected 1 reading, got %d", len(readings))
	}

	if readings[0].GatewayID != "gw-001" {
		t.Errorf("expected gateway_id gw-001, got %s", readings[0].GatewayID)
	}
	if readings[0].PowerWatts != 1200.0 {
		t.Errorf("expected power_watts 1200.0, got %f", readings[0].PowerWatts)
	}
}

func TestMemoryStore_GetLatest_EmptyGateway(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	readings, err := store.GetLatest(ctx, "nonexistent")
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if len(readings) != 0 {
		t.Fatalf("expected 0 readings, got %d", len(readings))
	}
}

func TestMemoryStore_EvictsOldReadings(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	// Store more than maxReadingsPerGateway entries.
	for i := 0; i < maxReadingsPerGateway+10; i++ {
		reading := TelemetryReading{
			GatewayID:  "gw-evict",
			PowerWatts: float64(i),
			Timestamp:  time.Now().UTC(),
		}
		if err := store.Store(ctx, reading); err != nil {
			t.Fatalf("unexpected error at iteration %d: %v", i, err)
		}
	}

	readings, err := store.GetLatest(ctx, "gw-evict")
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}

	if len(readings) != maxReadingsPerGateway {
		t.Fatalf("expected %d readings after eviction, got %d", maxReadingsPerGateway, len(readings))
	}

	// The oldest retained reading should be the 11th one (index 10).
	if readings[0].PowerWatts != 10.0 {
		t.Errorf("expected first retained reading to have power_watts 10.0, got %f", readings[0].PowerWatts)
	}
}

func TestMemoryStore_IsolatesGateways(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	store.Store(ctx, TelemetryReading{GatewayID: "gw-a", PowerWatts: 100, Timestamp: time.Now().UTC()})
	store.Store(ctx, TelemetryReading{GatewayID: "gw-b", PowerWatts: 200, Timestamp: time.Now().UTC()})

	readingsA, _ := store.GetLatest(ctx, "gw-a")
	readingsB, _ := store.GetLatest(ctx, "gw-b")

	if len(readingsA) != 1 || readingsA[0].PowerWatts != 100 {
		t.Errorf("gateway gw-a readings incorrect: %+v", readingsA)
	}
	if len(readingsB) != 1 || readingsB[0].PowerWatts != 200 {
		t.Errorf("gateway gw-b readings incorrect: %+v", readingsB)
	}
}
