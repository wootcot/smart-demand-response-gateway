/// Riverpod providers for the dashboard feature.
///
/// Uses `@riverpod` code generation annotations to produce type-safe,
/// auto-disposing providers for repository access and async grid status
/// fetching. The generated companion file `dashboard_providers.g.dart`
/// is produced by `build_runner` + `riverpod_generator`.
library;

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../data/dashboard_repository.dart';
import '../domain/models.dart';

part 'dashboard_providers.g.dart';

/// Provides the [DashboardRepository] instance for dependency injection.
///
/// Declared as a top-level `@riverpod` function rather than a class-based
/// provider because the repository is stateless and needs no lifecycle hooks.
/// Generated provider is accessed as `dashboardRepositoryProvider`.
@riverpod
DashboardRepository dashboardRepository(Ref ref) {
  // Instantiate without an injected HTTP client; tests override this provider
  // with a mock repository via ProviderScope.overrides.
  return DashboardRepository();
}

/// Fetches the current grid status from the backend.
///
/// Uses `ref.watch` on the repository provider to establish a reactive
/// dependency — if the repository provider is ever invalidated or overridden
/// (e.g., during testing), this provider automatically re-fetches.
/// Auto-disposes when no widget is watching, freeing network resources.
/// Generated provider is accessed as `gridStatusProvider`.
@riverpod
Future<GridStatus> gridStatus(Ref ref) async {
  final repository = ref.watch(dashboardRepositoryProvider);
  return repository.fetchGridStatus();
}
