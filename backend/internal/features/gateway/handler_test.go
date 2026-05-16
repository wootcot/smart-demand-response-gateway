package gateway

import (
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"
	"time"
)

func TestHandleGetStatus_EmptyGateways(t *testing.T) {
	store := NewMemoryStore()
	handler := NewHandler(store)

	req := httptest.NewRequest(http.MethodGet, "/status", nil)
	rec := httptest.NewRecorder()

	handler.HandleGetStatus(rec, req)

	if rec.Code != http.StatusOK {
		t.Fatalf("expected status 200, got %d", rec.Code)
	}

	var resp StatusResponse
	if err := json.NewDecoder(rec.Body).Decode(&resp); err != nil {
		t.Fatalf("failed to decode response: %v", err)
	}

	if len(resp.Gateways) != 0 {
		t.Errorf("expected 0 gateways, got %d", len(resp.Gateways))
	}

	if resp.TotalLoadWatts != 0 {
		t.Errorf("expected total load 0, got %f", resp.TotalLoadWatts)
	}

	if resp.PeakActive {
		t.Error("expected PeakActive to be false with no gateways")
	}
}

func TestHandleGetStatus_AggregatesLoad(t *testing.T) {
	store := NewMemoryStore()
	handler := NewHandler(store)

	now := time.Now().UTC()
	_ = store.UpdateStatus(nil, GatewayStatus{
		GatewayID: "gw-001", LastSeen: now, LastPowerWatts: 100.0, PeakStressActive: false,
	})
	_ = store.UpdateStatus(nil, GatewayStatus{
		GatewayID: "gw-002", LastSeen: now, LastPowerWatts: 250.5, PeakStressActive: false,
	})

	req := httptest.NewRequest(http.MethodGet, "/status", nil)
	rec := httptest.NewRecorder()

	handler.HandleGetStatus(rec, req)

	var resp StatusResponse
	if err := json.NewDecoder(rec.Body).Decode(&resp); err != nil {
		t.Fatalf("failed to decode response: %v", err)
	}

	expectedLoad := 350.5
	if resp.TotalLoadWatts != expectedLoad {
		t.Errorf("expected total load %f, got %f", expectedLoad, resp.TotalLoadWatts)
	}

	if resp.PeakActive {
		t.Error("expected PeakActive false when no gateway has peak stress")
	}
}

func TestHandleGetStatus_PeakActiveWhenAnyGatewayStressed(t *testing.T) {
	store := NewMemoryStore()
	handler := NewHandler(store)

	now := time.Now().UTC()
	_ = store.UpdateStatus(nil, GatewayStatus{
		GatewayID: "gw-001", LastSeen: now, LastPowerWatts: 100.0, PeakStressActive: false,
	})
	_ = store.UpdateStatus(nil, GatewayStatus{
		GatewayID: "gw-002", LastSeen: now, LastPowerWatts: 500.0, PeakStressActive: true,
	})

	req := httptest.NewRequest(http.MethodGet, "/status", nil)
	rec := httptest.NewRecorder()

	handler.HandleGetStatus(rec, req)

	var resp StatusResponse
	if err := json.NewDecoder(rec.Body).Decode(&resp); err != nil {
		t.Fatalf("failed to decode response: %v", err)
	}

	if !resp.PeakActive {
		t.Error("expected PeakActive true when at least one gateway has peak stress")
	}
}

func TestHandleGetStatus_ContentTypeJSON(t *testing.T) {
	store := NewMemoryStore()
	handler := NewHandler(store)

	req := httptest.NewRequest(http.MethodGet, "/status", nil)
	rec := httptest.NewRecorder()

	handler.HandleGetStatus(rec, req)

	ct := rec.Header().Get("Content-Type")
	if ct != "application/json" {
		t.Errorf("expected Content-Type application/json, got %s", ct)
	}
}
