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

import '../../../core/constants.dart';
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
      backgroundColor: AppColors.background,
      appBar: AppBar(
        title: const Text('Nepal Grid Dashboard'),
        backgroundColor: AppColors.primary,
        foregroundColor: Colors.white,
        actions: [
          _buildConnectionIndicator(connectionState.value),
        ],
      ),
      body: displayStatus != null
          ? _buildDashboardContent(context, displayStatus)
          : initialFetch.when(
              loading: () => const Center(
                child: CircularProgressIndicator(),
              ),
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
  Widget _buildConnectionIndicator(WebSocketConnectionState state) {
    final (color, tooltip) = switch (state) {
      WebSocketConnectionState.connected => (
          AppColors.gatewayOnline,
          'Connected'
        ),
      WebSocketConnectionState.connecting => (
          AppColors.gridElevated,
          'Connecting...'
        ),
      WebSocketConnectionState.disconnected => (
          AppColors.gatewayOffline,
          'Disconnected'
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
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(
              Icons.cloud_off,
              size: 64,
              color: AppColors.textSecondary,
            ),
            const SizedBox(height: 16),
            Text(
              'Connection Lost',
              style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                    color: AppColors.textPrimary,
                  ),
            ),
            const SizedBox(height: 8),
            Text(
              'Unable to reach the grid backend.\n$error',
              textAlign: TextAlign.center,
              style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                    color: AppColors.textSecondary,
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
    final loadColor = gridStatus.peakActive
        ? AppColors.gridPeakStress
        : gridStatus.totalLoadWatts > 5000
            ? AppColors.gridElevated
            : AppColors.gridNormal;

    return Card(
      color: AppColors.surface,
      elevation: 2,
      child: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Total Grid Load',
              style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    color: AppColors.textSecondary,
                  ),
            ),
            const SizedBox(height: 8),
            Row(
              crossAxisAlignment: CrossAxisAlignment.end,
              children: [
                Text(
                  gridStatus.totalLoadWatts.toStringAsFixed(1),
                  style: Theme.of(context).textTheme.headlineLarge?.copyWith(
                        color: loadColor,
                        fontWeight: FontWeight.bold,
                      ),
                ),
                const SizedBox(width: 4),
                Padding(
                  padding: const EdgeInsets.only(bottom: 4.0),
                  child: Text(
                    'W',
                    style: Theme.of(context).textTheme.titleLarge?.copyWith(
                          color: loadColor,
                        ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(
              '${gridStatus.gateways.length} gateway(s) reporting',
              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                    color: AppColors.textSecondary,
                  ),
            ),
          ],
        ),
      ),
    );
  }

  /// Displays a prominent peak stress indicator when load shedding is active.
  Widget _buildPeakStressIndicator(BuildContext context, bool peakActive) {
    return Card(
      color: peakActive ? AppColors.gridPeakStress : AppColors.gridNormal,
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
                    style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.white70,
                        ),
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
      BuildContext context, List<GatewayStatus> gateways) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 4.0),
          child: Text(
            'Connected Gateways',
            style: Theme.of(context).textTheme.titleMedium?.copyWith(
                  color: AppColors.textPrimary,
                ),
          ),
        ),
        const SizedBox(height: 8),
        if (gateways.isEmpty)
          Card(
            color: AppColors.surface,
            child: Padding(
              padding: const EdgeInsets.all(20.0),
              child: Center(
                child: Text(
                  'No gateways connected',
                  style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                        color: AppColors.textSecondary,
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
    final isOnline =
        DateTime.now().difference(gateway.lastSeen).inSeconds < 30;
    final statusColor =
        isOnline ? AppColors.gatewayOnline : AppColors.gatewayOffline;

    return Card(
      color: AppColors.surface,
      margin: const EdgeInsets.only(bottom: 8.0),
      child: ListTile(
        leading: CircleAvatar(
          backgroundColor: statusColor.withValues(alpha: 0.2),
          child: Icon(
            Icons.router,
            color: statusColor,
          ),
        ),
        title: Text(
          gateway.gatewayId,
          style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                color: AppColors.textPrimary,
                fontWeight: FontWeight.w500,
              ),
        ),
        subtitle: Text(
          '${gateway.lastPowerWatts.toStringAsFixed(1)} W',
          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                color: AppColors.textSecondary,
              ),
        ),
        trailing: gateway.peakStressActive
            ? const Icon(
                Icons.warning_amber_rounded,
                color: AppColors.gridPeakStress,
              )
            : Icon(
                Icons.check_circle_outline,
                color: statusColor,
              ),
      ),
    );
  }
}
