/// Riverpod providers for the dashboard feature.
///
/// Provides a WebSocket-based real-time connection to the backend for
/// grid status updates. Falls back to HTTP polling via [DashboardRepository]
/// for the initial load and as a recovery mechanism.
library;

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../data/dashboard_repository.dart';
import '../data/dashboard_websocket_service.dart';
import '../domain/models.dart';

part 'dashboard_providers.g.dart';

/// Provides the [DashboardRepository] instance for dependency injection.
///
/// Retained for initial data fetch and fallback scenarios where the
/// WebSocket connection has not yet delivered the first status update.
@riverpod
DashboardRepository dashboardRepository(Ref ref) {
  return DashboardRepository();
}

/// Provides the singleton [DashboardWebSocketService] instance.
///
/// Automatically connects on creation and disposes when no longer watched.
/// The service handles reconnection internally, so consumers only need
/// to listen to the streams it exposes.
@riverpod
DashboardWebSocketService dashboardWebSocketService(Ref ref) {
  final service = DashboardWebSocketService();
  service.connect();

  ref.onDispose(() {
    service.dispose();
  });

  return service;
}

/// Provides a stream of [GridStatus] updates from the WebSocket connection.
///
/// Widgets watching this provider rebuild automatically whenever the backend
/// pushes a new grid status frame. On initial subscription (before the first
/// WebSocket message arrives), falls back to an HTTP fetch for immediate data.
@riverpod
Stream<GridStatus> gridStatusStream(Ref ref) {
  final wsService = ref.watch(dashboardWebSocketServiceProvider);
  return wsService.statusStream;
}

/// Provides the current [WebSocketConnectionState] for UI indicators.
///
/// Allows the dashboard to show connection status (connected, connecting,
/// disconnected) without coupling presentation to the WebSocket service.
@riverpod
Stream<WebSocketConnectionState> connectionState(Ref ref) {
  final wsService = ref.watch(dashboardWebSocketServiceProvider);
  return wsService.connectionState;
}

/// Fetches the current grid status via HTTP as a one-shot operation.
///
/// Used for initial load before the WebSocket delivers its first message,
/// and as a manual refresh/retry mechanism.
@riverpod
Future<GridStatus> gridStatus(Ref ref) async {
  final repository = ref.watch(dashboardRepositoryProvider);
  return repository.fetchGridStatus();
}
