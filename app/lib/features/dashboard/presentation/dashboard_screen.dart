/// Dashboard presentation screen for the Nepal Grid Peak Load Controller.
///
/// Uses `HookConsumerWidget` from `hooks_riverpod` to combine Riverpod's
/// reactive provider system with flutter_hooks for local state management.
/// Displays real-time grid load status, connected gateway list, and peak
/// stress indicator via a persistent WebSocket connection to the backend.
library;

import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import '../../../core/theme.dart';
import '../data/dashboard_websocket_service.dart';
import '../domain/models.dart';
import '../providers/dashboard_providers.dart';

/// Main dashboard screen displaying grid telemetry and gateway status.
///
/// Connects to the backend via WebSocket for real-time updates. Shows a
/// connection indicator and falls back to HTTP fetch for initial data load.
class DashboardScreen extends HookConsumerWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    // Track the latest grid status received via WebSocket.
    final latestStatus = useState<GridStatus?>(null);
    final connectionState = useState(WebSocketConnectionState.connecting);
    final hasError = useState(false);

    // Watch the WebSocket stream for real-time grid status updates.
    final gridStatusStream = ref.watch(gridStatusStreamProvider);
    final connStateStream = ref.watch(connectionStateProvider);

    // Update local state from WebSocket stream.
    gridStatusStream.when(
      data: (status) {
        latestStatus.value = status;
        hasError.value = false;
      },
      loading: () {},
      error: (_, _) {},
    );

    // Update connection state from stream.
    connStateStream.when(
      data: (state) {
        connectionState.value = state;
        if (state == WebSocketConnectionState.disconnected) {
          // Only show error if we never received data.
          if (latestStatus.value == null) {
            hasError.value = true;
          }
        }
      },
      loading: () {},
      error: (_, _) {},
    );

    // On first load, fetch via HTTP to show data immediately while
    // WebSocket connection is being established.
    final initialFetch = ref.watch(gridStatusProvider);

    // Use WebSocket data if available, otherwise fall back to HTTP fetch.
    final displayStatus = latestStatus.value;

    return Scaffold(
      appBar: AppBar(
        title: const Text('Nepal Grid Dashboard'),
        actions: [_buildConnectionIndicator(context, connectionState.value)],
      ),
      body: displayStatus != null
          ? _buildDashboardContent(context, displayStatus)
          : initialFetch.when(
              loading: () => const Center(child: CircularProgressIndicator()),
              error: (error, _) => _buildErrorState(context, ref, error),
              data: (gridStatus) {
                // Seed local state with HTTP response until WebSocket takes over.
                WidgetsBinding.instance.addPostFrameCallback((_) {
                  if (latestStatus.value == null) {
                    latestStatus.value = gridStatus;
                  }
                });
                return _buildDashboardContent(context, gridStatus);
              },
            ),
    );
  }

  /// Builds a small connection status indicator in the app bar.
  Widget _buildConnectionIndicator(
    BuildContext context,
    WebSocketConnectionState state,
  ) {
    final gridColors = Theme.of(context).extension<GridColors>()!;

    final (color, tooltip) = switch (state) {
      WebSocketConnectionState.connected => (
        gridColors.gatewayOnline,
        'Connected',
      ),
      WebSocketConnectionState.connecting => (
        gridColors.gridElevated,
        'Connecting...',
      ),
      WebSocketConnectionState.disconnected => (
        gridColors.gatewayOffline,
        'Disconnected',
      ),
    };

    return Padding(
      padding: const EdgeInsets.only(right: 16.0),
      child: Tooltip(
        message: tooltip,
        child: Icon(Icons.circle, color: color, size: 12),
      ),
    );
  }

  /// Builds the error state UI with a retry action.
  Widget _buildErrorState(BuildContext context, WidgetRef ref, Object error) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.cloud_off,
              size: 64,
              color: colorScheme.onSurfaceVariant,
            ),
            const SizedBox(height: 16),
            Text(
              'Connection Lost',
              style: theme.textTheme.headlineSmall?.copyWith(
                color: colorScheme.onSurface,
              ),
            ),
            const SizedBox(height: 8),
            Text(
              'Unable to reach the grid backend.\n$error',
              textAlign: TextAlign.center,
              style: theme.textTheme.bodyMedium?.copyWith(
                color: colorScheme.onSurfaceVariant,
              ),
            ),
            const SizedBox(height: 24),
            ElevatedButton.icon(
              onPressed: () {
                ref.invalidate(gridStatusProvider);
                ref.invalidate(dashboardWebSocketServiceProvider);
              },
              icon: const Icon(Icons.refresh),
              label: const Text('Retry'),
            ),
          ],
        ),
      ),
    );
  }

  /// Builds the main dashboard content with grid load, peak indicator, and gateway list.
  Widget _buildDashboardContent(BuildContext context, GridStatus gridStatus) {
    return RefreshIndicator(
      onRefresh: () async {},
      child: ListView(
        padding: const EdgeInsets.all(16.0),
        children: [
          _buildGridLoadCard(context, gridStatus),
          const SizedBox(height: 16),
          _buildPeakStressIndicator(context, gridStatus.peakActive),
          const SizedBox(height: 16),
          _buildGatewaysSection(context, gridStatus.gateways),
        ],
      ),
    );
  }

  /// Displays the total grid load with semantic coloring based on load level.
  Widget _buildGridLoadCard(BuildContext context, GridStatus gridStatus) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final gridColors = theme.extension<GridColors>()!;

    final loadColor = gridStatus.peakActive
        ? gridColors.gridPeakStress
        : gridStatus.totalLoadWatts > 5000
        ? gridColors.gridElevated
        : gridColors.gridNormal;

    return Card(
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Total Grid Load',
              style: theme.textTheme.titleMedium?.copyWith(
                color: colorScheme.onSurfaceVariant,
              ),
            ),
            const SizedBox(height: 8),
            Row(
              crossAxisAlignment: CrossAxisAlignment.end,
              children: [
                Text(
                  gridStatus.totalLoadWatts.toStringAsFixed(1),
                  style: theme.textTheme.headlineLarge?.copyWith(
                    color: loadColor,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const SizedBox(width: 4),
                Padding(
                  padding: const EdgeInsets.only(bottom: 4.0),
                  child: Text(
                    'W',
                    style: theme.textTheme.titleLarge?.copyWith(
                      color: loadColor,
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(
              '${gridStatus.gateways.length} gateway(s) reporting',
              style: theme.textTheme.bodySmall?.copyWith(
                color: colorScheme.onSurfaceVariant,
              ),
            ),
          ],
        ),
      ),
    );
  }

  /// Displays a prominent peak stress indicator when load shedding is active.
  Widget _buildPeakStressIndicator(BuildContext context, bool peakActive) {
    final gridColors = Theme.of(context).extension<GridColors>()!;

    return Card(
      color: peakActive ? gridColors.gridPeakStress : gridColors.gridNormal,
      elevation: peakActive ? 4 : 1,
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 20.0, vertical: 16.0),
        child: Row(
          children: [
            Icon(
              peakActive ? Icons.warning_amber_rounded : Icons.check_circle,
              color: Colors.white,
              size: 32,
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    peakActive ? 'Peak Stress Active' : 'Grid Normal',
                    style: Theme.of(context).textTheme.titleMedium?.copyWith(
                      color: Colors.white,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 2),
                  Text(
                    peakActive
                        ? 'Load shedding in effect'
                        : 'Operating within normal parameters',
                    style: Theme.of(
                      context,
                    ).textTheme.bodySmall?.copyWith(color: Colors.white70),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  /// Builds the connected gateways list section.
  Widget _buildGatewaysSection(
    BuildContext context,
    List<GatewayStatus> gateways,
  ) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 4.0),
          child: Text(
            'Connected Gateways',
            style: theme.textTheme.titleMedium?.copyWith(
              color: colorScheme.onSurface,
            ),
          ),
        ),
        const SizedBox(height: 8),
        if (gateways.isEmpty)
          Card(
            child: Padding(
              padding: const EdgeInsets.all(20.0),
              child: Center(
                child: Text(
                  'No gateways connected',
                  style: theme.textTheme.bodyMedium?.copyWith(
                    color: colorScheme.onSurfaceVariant,
                  ),
                ),
              ),
            ),
          )
        else
          ...gateways.map((gateway) => _buildGatewayTile(context, gateway)),
      ],
    );
  }

  /// Builds an individual gateway status tile.
  Widget _buildGatewayTile(BuildContext context, GatewayStatus gateway) {
    final theme = Theme.of(context);
    final colorScheme = theme.colorScheme;
    final gridColors = theme.extension<GridColors>()!;

    final isOnline = DateTime.now().difference(gateway.lastSeen).inSeconds < 30;
    final statusColor = isOnline
        ? gridColors.gatewayOnline
        : gridColors.gatewayOffline;

    return Card(
      margin: const EdgeInsets.only(bottom: 8.0),
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: statusColor.withValues(alpha: 0.2),
          child: Icon(Icons.router, color: statusColor),
        ),
        title: Text(
          gateway.gatewayId,
          style: theme.textTheme.bodyLarge?.copyWith(
            color: colorScheme.onSurface,
            fontWeight: FontWeight.w500,
          ),
        ),
        subtitle: Text(
          '${gateway.lastPowerWatts.toStringAsFixed(1)} W',
          style: theme.textTheme.bodySmall?.copyWith(
            color: colorScheme.onSurfaceVariant,
          ),
        ),
        trailing: gateway.peakStressActive
            ? Icon(
                Icons.warning_amber_rounded,
                color: gridColors.gridPeakStress,
              )
            : Icon(Icons.check_circle_outline, color: statusColor),
      ),
    );
  }
}
