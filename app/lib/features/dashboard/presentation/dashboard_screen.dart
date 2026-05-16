/// Dashboard presentation screen for the Nepal Grid Peak Load Controller.
///
/// Uses `HookConsumerWidget` from `hooks_riverpod` to combine Riverpod's
/// reactive provider system with flutter_hooks for local state and lifecycle
/// management. Displays real-time grid load status, connected gateway list,
/// and peak stress indicator with periodic polling via `useEffect`.
library;

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import '../../../core/constants.dart';
import '../domain/models.dart';
import '../providers/dashboard_providers.dart';

/// Main dashboard screen displaying grid telemetry and gateway status.
///
/// Extends `HookConsumerWidget` to leverage both Riverpod's `ref.watch()`
/// for reactive provider access and flutter_hooks (`useEffect`, `useState`)
/// for timer-based polling and local ephemeral state.
class DashboardScreen extends HookConsumerWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    // Local state tracking whether peak stress polling is active.
    final isPeakActive = useState(false);

    // Periodic refresh using useEffect to set up a timer-based polling loop.
    // The interval shortens during peak stress events for more responsive updates.
    useEffect(() {
      final interval =
          isPeakActive.value ? peakStressPollInterval : gridStatusPollInterval;

      final timer = Timer.periodic(interval, (_) {
        // Invalidate the provider to trigger a fresh fetch from the backend.
        ref.invalidate(gridStatusProvider);
      });

      return timer.cancel;
    }, [isPeakActive.value]);

    // Watch the grid status provider for reactive rebuilds on data changes.
    final gridStatusAsync = ref.watch(gridStatusProvider);

    return Scaffold(
      backgroundColor: AppColors.background,
      appBar: AppBar(
        title: const Text('Nepal Grid Dashboard'),
        backgroundColor: AppColors.primary,
        foregroundColor: Colors.white,
      ),
      body: gridStatusAsync.when(
        loading: () => const Center(
          child: CircularProgressIndicator(),
        ),
        error: (error, stackTrace) => _buildErrorState(context, ref, error),
        data: (gridStatus) {
          // Sync local peak state with server response for polling interval adjustment.
          WidgetsBinding.instance.addPostFrameCallback((_) {
            if (isPeakActive.value != gridStatus.peakActive) {
              isPeakActive.value = gridStatus.peakActive;
            }
          });

          return _buildDashboardContent(context, gridStatus);
        },
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
              onPressed: () => ref.invalidate(gridStatusProvider),
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
    // Determine color based on grid state: peak stress > elevated > normal.
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
    // Consider a gateway offline if not seen in the last 30 seconds.
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
