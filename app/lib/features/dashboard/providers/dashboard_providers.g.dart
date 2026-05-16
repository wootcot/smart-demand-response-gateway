// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'dashboard_providers.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

String _$dashboardRepositoryHash() =>
    r'df00eb896ff0a6892ce60a05a9e0984d3b04d94f';

/// Provides the DashboardRepository instance for dependency injection.
/// Generated provider is accessed as `dashboardRepositoryProvider`.
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
String _$gridStatusHash() => r'f54b25df6d150a2f8c85527cf5957fafea0c8b14';

/// Fetches the current grid status from the backend.
/// Automatically disposes when no longer watched.
/// Generated provider is accessed as `gridStatusProvider`.
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
