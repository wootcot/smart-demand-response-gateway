// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'dashboard_providers.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

String _$dashboardRepositoryHash() =>
    r'df00eb896ff0a6892ce60a05a9e0984d3b04d94f';

/// Provides the [DashboardRepository] instance for dependency injection.
///
/// Retained for initial data fetch and fallback scenarios where the
/// WebSocket connection has not yet delivered the first status update.
///
/// Copied from [dashboardRepository].
@ProviderFor(dashboardRepository)
final dashboardRepositoryProvider =
    AutoDisposeProvider<DashboardRepository>.internal(
      dashboardRepository,
      name: r'dashboardRepositoryProvider',
      debugGetCreateSourceHash: const bool.fromEnvironment('dart.vm.product')
          ? null
          : _$dashboardRepositoryHash,
      dependencies: null,
      allTransitiveDependencies: null,
    );

@Deprecated('Will be removed in 3.0. Use Ref instead')
// ignore: unused_element
typedef DashboardRepositoryRef = AutoDisposeProviderRef<DashboardRepository>;
String _$dashboardWebSocketServiceHash() =>
    r'11fbe6067c7b87b12131894308ff6966bef9b1a6';

/// Provides the singleton [DashboardWebSocketService] instance.
///
/// Automatically connects on creation and disposes when no longer watched.
/// The service handles reconnection internally, so consumers only need
/// to listen to the streams it exposes.
///
/// Copied from [dashboardWebSocketService].
@ProviderFor(dashboardWebSocketService)
final dashboardWebSocketServiceProvider =
    AutoDisposeProvider<DashboardWebSocketService>.internal(
      dashboardWebSocketService,
      name: r'dashboardWebSocketServiceProvider',
      debugGetCreateSourceHash: const bool.fromEnvironment('dart.vm.product')
          ? null
          : _$dashboardWebSocketServiceHash,
      dependencies: null,
      allTransitiveDependencies: null,
    );

@Deprecated('Will be removed in 3.0. Use Ref instead')
// ignore: unused_element
typedef DashboardWebSocketServiceRef =
    AutoDisposeProviderRef<DashboardWebSocketService>;
String _$gridStatusStreamHash() => r'e62249ab60a2a3e67e5262af834ba62a7054fe62';

/// Provides a stream of [GridStatus] updates from the WebSocket connection.
///
/// Widgets watching this provider rebuild automatically whenever the backend
/// pushes a new grid status frame. On initial subscription (before the first
/// WebSocket message arrives), falls back to an HTTP fetch for immediate data.
///
/// Copied from [gridStatusStream].
@ProviderFor(gridStatusStream)
final gridStatusStreamProvider = AutoDisposeStreamProvider<GridStatus>.internal(
  gridStatusStream,
  name: r'gridStatusStreamProvider',
  debugGetCreateSourceHash: const bool.fromEnvironment('dart.vm.product')
      ? null
      : _$gridStatusStreamHash,
  dependencies: null,
  allTransitiveDependencies: null,
);

@Deprecated('Will be removed in 3.0. Use Ref instead')
// ignore: unused_element
typedef GridStatusStreamRef = AutoDisposeStreamProviderRef<GridStatus>;
String _$connectionStateHash() => r'b68cc7ba7d0531bfefe0dea48615ef074011b541';

/// Provides the current [WebSocketConnectionState] for UI indicators.
///
/// Allows the dashboard to show connection status (connected, connecting,
/// disconnected) without coupling presentation to the WebSocket service.
///
/// Copied from [connectionState].
@ProviderFor(connectionState)
final connectionStateProvider =
    AutoDisposeStreamProvider<WebSocketConnectionState>.internal(
      connectionState,
      name: r'connectionStateProvider',
      debugGetCreateSourceHash: const bool.fromEnvironment('dart.vm.product')
          ? null
          : _$connectionStateHash,
      dependencies: null,
      allTransitiveDependencies: null,
    );

@Deprecated('Will be removed in 3.0. Use Ref instead')
// ignore: unused_element
typedef ConnectionStateRef =
    AutoDisposeStreamProviderRef<WebSocketConnectionState>;
String _$gridStatusHash() => r'f54b25df6d150a2f8c85527cf5957fafea0c8b14';

/// Fetches the current grid status via HTTP as a one-shot operation.
///
/// Used for initial load before the WebSocket delivers its first message,
/// and as a manual refresh/retry mechanism.
///
/// Copied from [gridStatus].
@ProviderFor(gridStatus)
final gridStatusProvider = AutoDisposeFutureProvider<GridStatus>.internal(
  gridStatus,
  name: r'gridStatusProvider',
  debugGetCreateSourceHash: const bool.fromEnvironment('dart.vm.product')
      ? null
      : _$gridStatusHash,
  dependencies: null,
  allTransitiveDependencies: null,
);

@Deprecated('Will be removed in 3.0. Use Ref instead')
// ignore: unused_element
typedef GridStatusRef = AutoDisposeFutureProviderRef<GridStatus>;
// ignore_for_file: type=lint
// ignore_for_file: subtype_of_sealed_class, invalid_use_of_internal_member, invalid_use_of_visible_for_testing_member, deprecated_member_use_from_same_package
