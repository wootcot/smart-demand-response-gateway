/// WebSocket service for real-time dashboard updates.
///
/// Maintains a persistent WebSocket connection to the backend's
/// `/ws/dashboard` endpoint. The backend pushes grid status updates
/// whenever gateway telemetry arrives, eliminating the need for polling.
/// Includes automatic reconnection with exponential backoff on disconnection.
library;

import 'dart:async';
import 'dart:convert';

import 'package:web_socket_channel/web_socket_channel.dart';

import '../../../core/constants.dart';
import '../domain/models.dart';

/// Manages the WebSocket connection to the backend dashboard endpoint.
///
/// Exposes a [Stream<GridStatus>] that emits new grid status snapshots
/// in real time. Handles connection lifecycle, reconnection, and cleanup.
class DashboardWebSocketService {
  WebSocketChannel? _channel;
  Timer? _reconnectTimer;
  final _statusController = StreamController<GridStatus>.broadcast();
  final _connectionStateController =
      StreamController<WebSocketConnectionState>.broadcast();
  bool _disposed = false;
  bool _intentionalClose = false;

  /// Stream of grid status updates pushed by the backend.
  Stream<GridStatus> get statusStream => _statusController.stream;

  /// Stream of connection state changes for UI feedback.
  Stream<WebSocketConnectionState> get connectionState =>
      _connectionStateController.stream;

  /// Establishes the WebSocket connection to the backend.
  void connect() {
    if (_disposed) return;
    _intentionalClose = false;
    _connectionStateController.add(WebSocketConnectionState.connecting);

    try {
      final uri = Uri.parse(dashboardWsUrl);
      _channel = WebSocketChannel.connect(uri);

      _channel!.ready.then((_) {
        if (_disposed) return;
        _connectionStateController.add(WebSocketConnectionState.connected);
      }).catchError((Object error) {
        if (_disposed) return;
        _connectionStateController.add(WebSocketConnectionState.disconnected);
        _scheduleReconnect();
      });

      _channel!.stream.listen(
        _onMessage,
        onError: _onError,
        onDone: _onDone,
        cancelOnError: false,
      );
    } catch (e) {
      _connectionStateController.add(WebSocketConnectionState.disconnected);
      _scheduleReconnect();
    }
  }

  void _onMessage(dynamic message) {
    if (_disposed) return;

    try {
      final json = jsonDecode(message as String) as Map<String, dynamic>;
      final type = json['type'] as String?;

      if (type == 'dashboard_status') {
        final status = GridStatus.fromJson(json);
        _statusController.add(status);
      }
    } catch (e) {
      // Ignore malformed messages — don't crash the stream.
    }
  }

  void _onError(Object error) {
    if (_disposed) return;
    _connectionStateController.add(WebSocketConnectionState.disconnected);
    _scheduleReconnect();
  }

  void _onDone() {
    if (_disposed || _intentionalClose) return;
    _connectionStateController.add(WebSocketConnectionState.disconnected);
    _scheduleReconnect();
  }

  void _scheduleReconnect() {
    if (_disposed || _intentionalClose) return;
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(wsReconnectDelay, () {
      if (!_disposed && !_intentionalClose) {
        connect();
      }
    });
  }

  /// Closes the WebSocket connection and releases resources.
  void dispose() {
    _disposed = true;
    _intentionalClose = true;
    _reconnectTimer?.cancel();
    _channel?.sink.close();
    _statusController.close();
    _connectionStateController.close();
  }
}

/// Represents the current state of the WebSocket connection.
enum WebSocketConnectionState {
  /// Attempting to establish connection.
  connecting,

  /// Connection is active and receiving data.
  connected,

  /// Connection lost or not yet established.
  disconnected,
}
