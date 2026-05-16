/// Domain models for the dashboard feature.
///
/// Contains data classes representing telemetry readings, gateway operational
/// states, and overall grid status. Each model includes a fromJson factory
/// constructor for deserializing backend API responses.
library;

/// Represents a telemetry reading from a single gateway.
class TelemetryReading {
  final String gatewayId;
  final double powerWatts;
  final DateTime timestamp;

  const TelemetryReading({
    required this.gatewayId,
    required this.powerWatts,
    required this.timestamp,
  });

  /// Deserializes from the backend JSON contract.
  ///
  /// Uses `num.toDouble()` rather than direct `double` cast because the Go
  /// backend may serialize integer watt values without a decimal point,
  /// which Dart's JSON decoder represents as `int` rather than `double`.
  factory TelemetryReading.fromJson(Map<String, dynamic> json) {
    return TelemetryReading(
      gatewayId: json['gateway_id'] as String,
      powerWatts: (json['power_watts'] as num).toDouble(),
      timestamp: DateTime.parse(json['timestamp'] as String),
    );
  }
}

/// Represents the operational state of a single gateway.
class GatewayStatus {
  final String gatewayId;
  final DateTime lastSeen;
  final double lastPowerWatts;
  final bool peakStressActive;

  const GatewayStatus({
    required this.gatewayId,
    required this.lastSeen,
    required this.lastPowerWatts,
    required this.peakStressActive,
  });

  /// Deserializes from the backend JSON contract.
  ///
  /// Field names use snake_case to match the Go backend's JSON serialization
  /// convention, avoiding the need for a separate mapping layer.
  factory GatewayStatus.fromJson(Map<String, dynamic> json) {
    return GatewayStatus(
      gatewayId: json['gateway_id'] as String,
      lastSeen: DateTime.parse(json['last_seen'] as String),
      lastPowerWatts: (json['last_power_watts'] as num).toDouble(),
      peakStressActive: json['peak_stress_active'] as bool,
    );
  }
}

/// Represents the overall grid status returned by the backend.
class GridStatus {
  final List<GatewayStatus> gateways;
  final double totalLoadWatts;
  final bool peakActive;

  const GridStatus({
    required this.gateways,
    required this.totalLoadWatts,
    required this.peakActive,
  });

  /// Deserializes the aggregate grid response from the backend.
  ///
  /// The backend computes `total_load_watts` server-side rather than summing
  /// individual gateway readings on the client, ensuring consistency across
  /// all dashboard instances viewing the same data.
  factory GridStatus.fromJson(Map<String, dynamic> json) {
    return GridStatus(
      gateways: (json['gateways'] as List)
          .map((g) => GatewayStatus.fromJson(g as Map<String, dynamic>))
          .toList(),
      totalLoadWatts: (json['total_load_watts'] as num).toDouble(),
      peakActive: json['peak_active'] as bool,
    );
  }
}
