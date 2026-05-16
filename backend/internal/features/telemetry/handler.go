// Package telemetry implements the telemetry ingestion feature for the demand-response gateway.
// It receives power readings from ESP32 gateways and stores them for aggregation.
// This package depends only on the shared package and standard library, maintaining
// feature isolation per the Clean Architecture dependency rule.
package telemetry

import (
	"encoding/json"
	"log/slog"
	"net/http"
	"time"
)

// TelemetryRequest is the payload sent by the gateway on each 10-second
// network sync cycle. The gateway reports its smoothed power reading.
type TelemetryRequest struct {
	GatewayID  string  `json:"gateway_id"`
	PowerWatts float64 `json:"power_watts"`
}

// TelemetryResponse is returned to the gateway after telemetry ingestion.
// The peak_stress_active flag instructs the gateway whether to commence
// appliance shedding on its next control cycle.
type TelemetryResponse struct {
	Acknowledged     bool `json:"acknowledged"`
	PeakStressActive bool `json:"peak_stress_active"`
}

// Handler provides HTTP endpoints for the telemetry feature.
// It depends on the TelemetryRepository interface for persistence,
// enabling easy substitution of storage backends.
type Handler struct {
	repo   TelemetryRepository
	logger *slog.Logger
}

// NewHandler creates a telemetry handler with the given repository and logger.
func NewHandler(repo TelemetryRepository, logger *slog.Logger) *Handler {
	return &Handler{
		repo:   repo,
		logger: logger,
	}
}

// RegisterRoutes mounts the telemetry endpoints on the provided ServeMux.
func (h *Handler) RegisterRoutes(mux *http.ServeMux) {
	mux.HandleFunc("POST /telemetry", h.handlePostTelemetry)
}

// HandlePostTelemetry is the exported entry point for the POST /telemetry route,
// enabling the composition root to wire this handler via HandlerRegistration.
func (h *Handler) HandlePostTelemetry(w http.ResponseWriter, r *http.Request) {
	h.handlePostTelemetry(w, r)
}

// handlePostTelemetry processes incoming power readings from gateways.
// It validates the request payload, stores the reading, and returns the
// current peak stress status. The peak_stress_active flag is currently
// hardcoded to false; a future peak detection feature will compute this
// dynamically based on aggregate grid load.
func (h *Handler) handlePostTelemetry(w http.ResponseWriter, r *http.Request) {
	var req TelemetryRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		h.logger.Warn("invalid telemetry payload", "error", err)
		http.Error(w, `{"error":"invalid request body"}`, http.StatusBadRequest)
		return
	}

	// Validate required fields to reject malformed gateway submissions early.
	if req.GatewayID == "" {
		h.logger.Warn("missing gateway_id in telemetry request")
		http.Error(w, `{"error":"gateway_id is required"}`, http.StatusBadRequest)
		return
	}
	if req.PowerWatts < 0 {
		h.logger.Warn("negative power_watts in telemetry request", "gateway_id", req.GatewayID)
		http.Error(w, `{"error":"power_watts must be non-negative"}`, http.StatusBadRequest)
		return
	}

	reading := TelemetryReading{
		GatewayID:  req.GatewayID,
		PowerWatts: req.PowerWatts,
		Timestamp:  time.Now().UTC(),
	}

	if err := h.repo.Store(r.Context(), reading); err != nil {
		h.logger.Error("failed to store telemetry reading", "error", err, "gateway_id", req.GatewayID)
		http.Error(w, `{"error":"internal server error"}`, http.StatusInternalServerError)
		return
	}

	h.logger.Info("telemetry stored", "gateway_id", req.GatewayID, "power_watts", req.PowerWatts)

	resp := TelemetryResponse{
		Acknowledged:     true,
		PeakStressActive: false,
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(resp)
}
