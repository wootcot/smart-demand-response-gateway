// Package gateway provides the gateway status feature for the demand-response system.
// It tracks the operational state of connected ESP32 gateways, including their last
// reported power readings and peak stress status. This package is self-contained with
// its own domain types, repository interface, and HTTP handler — it depends only on
// the shared package and the standard library.
package gateway

import (
	"encoding/json"
	"net/http"
)

// StatusResponse is the JSON payload returned to the Flutter dashboard for grid overview.
// It aggregates all gateway statuses, computes the total grid load, and indicates whether
// any gateway is currently under peak stress — enabling the dashboard to render a unified
// grid health view.
type StatusResponse struct {
	Gateways       []GatewayStatus `json:"gateways"`
	TotalLoadWatts float64         `json:"total_load_watts"`
	PeakActive     bool            `json:"peak_active"`
}

// Handler provides HTTP endpoints for the gateway feature.
// It depends on the GatewayRepository interface for data access, allowing the
// composition root to inject either an in-memory store or a persistent backend.
type Handler struct {
	repo GatewayRepository
}

// NewHandler creates a Handler with the given repository implementation.
func NewHandler(repo GatewayRepository) *Handler {
	return &Handler{repo: repo}
}

// HandleGetStatus serves GET /status requests by aggregating all gateway statuses,
// computing the total grid load as the sum of each gateway's last reported power,
// and determining whether peak stress is active on any gateway. This endpoint is
// polled by the Flutter dashboard for real-time grid visualization.
func (h *Handler) HandleGetStatus(w http.ResponseWriter, r *http.Request) {
	statuses, err := h.repo.GetAll(r.Context())
	if err != nil {
		http.Error(w, "failed to retrieve gateway statuses", http.StatusInternalServerError)
		return
	}

	var totalLoad float64
	var peakActive bool
	for _, gs := range statuses {
		totalLoad += gs.LastPowerWatts
		if gs.PeakStressActive {
			peakActive = true
		}
	}

	resp := StatusResponse{
		Gateways:       statuses,
		TotalLoadWatts: totalLoad,
		PeakActive:     peakActive,
	}

	w.Header().Set("Content-Type", "application/json")
	if err := json.NewEncoder(w).Encode(resp); err != nil {
		http.Error(w, "failed to encode response", http.StatusInternalServerError)
		return
	}
}
