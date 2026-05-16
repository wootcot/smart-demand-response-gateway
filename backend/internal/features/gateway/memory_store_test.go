package gateway

import (
	"context"
	"testing"
	"time"
)

func TestMemoryStore_UpdateStatus(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	status := GatewayStatus{
		GatewayID:        "gw-001",
		LastSeen:         time.Now().UTC(),
		LastPowerWatts:   450.5,
		PeakStressActive: false,
	}

	if err := store.UpdateStatus(ctx, status); err != nil {
		t.Fatalf("UpdateStatus returned unexpected error: %v", err)
	}

	all, err := store.GetAll(ctx)
	if err != nil {
		t.Fatalf("GetAll returned unexpected error: %v", err)
	}

	if len(all) != 1 {
		t.Fatalf("expected 1 gateway, got %d", len(all))
	}

	if all[0].GatewayID != "gw-001" {
		t.Errorf("expected gateway ID gw-001, got %s", all[0].GatewayID)
	}

	if all[0].LastPowerWatts != 450.5 {
		t.Errorf("expected power 450.5, got %f", all[0].LastPowerWatts)
	}
}

func TestMemoryStore_UpdateStatus_Overwrites(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	first := GatewayStatus{
		GatewayID:        "gw-001",
		LastSeen:         time.Now().UTC(),
		LastPowerWatts:   100.0,
		PeakStressActive: false,
	}
	second := GatewayStatus{
		GatewayID:        "gw-001",
		LastSeen:         time.Now().UTC().Add(10 * time.Second),
		LastPowerWatts:   200.0,
		PeakStressActive: true,
	}

	_ = store.UpdateStatus(ctx, first)
	_ = store.UpdateStatus(ctx, second)

	all, _ := store.GetAll(ctx)
	if len(all) != 1 {
		t.Fatalf("expected 1 gateway after overwrite, got %d", len(all))
	}

	if all[0].LastPowerWatts != 200.0 {
		t.Errorf("expected overwritten power 200.0, got %f", all[0].LastPowerWatts)
	}

	if !all[0].PeakStressActive {
		t.Error("expected PeakStressActive to be true after overwrite")
	}
}

func TestMemoryStore_GetAll_EmptySlice(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	all, err := store.GetAll(ctx)
	if err != nil {
		t.Fatalf("GetAll returned unexpected error: %v", err)
	}

	if all == nil {
		t.Fatal("expected empty slice, got nil")
	}

	if len(all) != 0 {
		t.Fatalf("expected 0 gateways, got %d", len(all))
	}
}

func TestMemoryStore_MultipleGateways(t *testing.T) {
	store := NewMemoryStore()
	ctx := context.Background()

	gateways := []GatewayStatus{
		{GatewayID: "gw-001", LastSeen: time.Now().UTC(), LastPowerWatts: 100.0, PeakStressActive: false},
		{GatewayID: "gw-002", LastSeen: time.Now().UTC(), LastPowerWatts: 250.0, PeakStressActive: true},
		{GatewayID: "gw-003", LastSeen: time.Now().UTC(), LastPowerWatts: 75.0, PeakStressActive: false},
	}

	for _, gs := range gateways {
		if err := store.UpdateStatus(ctx, gs); err != nil {
			t.Fatalf("UpdateStatus for %s returned error: %v", gs.GatewayID, err)
		}
	}

	all, err := store.GetAll(ctx)
	if err != nil {
		t.Fatalf("GetAll returned unexpected error: %v", err)
	}

	if len(all) != 3 {
		t.Fatalf("expected 3 gateways, got %d", len(all))
	}
}
