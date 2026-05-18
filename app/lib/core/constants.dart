/// Core application constants for the Nepal Grid Peak Load Controller dashboard.
///
/// Centralizes API configuration, polling intervals, and color palette tokens
/// so that feature modules reference a single source of truth for shared values.
library;

import 'package:flutter/material.dart';

/// Base URL for the backend API server.
///
/// The Go backend listens on port 8080 by default. In production, this would
/// be replaced with the deployed server address via environment configuration.
const String apiBaseUrl = 'http://localhost:8080';

/// WebSocket URL for real-time dashboard status updates.
///
/// Connects to the dedicated dashboard WebSocket endpoint which pushes
/// grid status updates whenever gateway telemetry arrives.
const String dashboardWsUrl = 'ws://localhost:8080/ws/dashboard';

/// Polling interval for fetching grid status from the backend.
///
/// Matches the firmware network task's 10-second telemetry push cycle,
/// ensuring the dashboard stays in sync with the latest gateway readings.
const Duration gridStatusPollInterval = Duration(seconds: 10);

/// Shorter polling interval used during peak stress events for more
/// responsive UI updates when load shedding decisions are active.
const Duration peakStressPollInterval = Duration(seconds: 3);

/// HTTP request timeout for backend API calls.
///
/// Prevents the UI from hanging indefinitely if the backend is unreachable.
const Duration httpRequestTimeout = Duration(seconds: 5);

/// Delay before attempting to reconnect the dashboard WebSocket after a
/// disconnection. Prevents aggressive reconnection loops on transient failures.
const Duration wsReconnectDelay = Duration(seconds: 3);

/// Color palette tokens for the Nepal Grid Peak Load Controller dashboard.
///
/// Uses a semantic naming scheme tied to grid operational states so that
/// widgets communicate system health at a glance without decoding raw values.
abstract final class AppColors {
  /// Primary brand color — deep blue representing stable grid operation.
  static const Color primary = Color(0xFF1565C0);

  /// Secondary accent — teal for interactive elements and highlights.
  static const Color secondary = Color(0xFF00897B);

  /// Grid status: normal load — green indicating healthy operation.
  static const Color gridNormal = Color(0xFF4CAF50);

  /// Grid status: elevated load — amber warning before peak stress.
  static const Color gridElevated = Color(0xFFFFA726);

  /// Grid status: peak stress active — red indicating load shedding in effect.
  static const Color gridPeakStress = Color(0xFFE53935);

  /// Background color for the dashboard scaffold.
  static const Color background = Color(0xFFF5F5F5);

  /// Surface color for cards and elevated containers.
  static const Color surface = Color(0xFFFFFFFF);

  /// Text color for primary content.
  static const Color textPrimary = Color(0xFF212121);

  /// Text color for secondary/muted content.
  static const Color textSecondary = Color(0xFF757575);

  /// Gateway online indicator.
  static const Color gatewayOnline = Color(0xFF66BB6A);

  /// Gateway offline indicator.
  static const Color gatewayOffline = Color(0xFFBDBDBD);
}
