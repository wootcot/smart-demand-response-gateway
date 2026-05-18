/// Application theme configuration for the Nepal Grid Peak Load Controller.
///
/// Defines the [MaterialApp] [ThemeData] and a custom [ThemeExtension] for
/// domain-specific colors (grid states, gateway status) that don't map to
/// standard Material [ColorScheme] properties.
library;

import 'package:flutter/material.dart';

import 'constants.dart';

/// Custom theme extension for grid-specific semantic colors.
///
/// These colors represent operational states of the power grid and connected
/// gateways. They are accessed via `Theme.of(context).extension<GridColors>()`.
@immutable
class GridColors extends ThemeExtension<GridColors> {
  const GridColors({
    required this.gridNormal,
    required this.gridElevated,
    required this.gridPeakStress,
    required this.gatewayOnline,
    required this.gatewayOffline,
  });

  /// Grid status: normal load — green indicating healthy operation.
  final Color gridNormal;

  /// Grid status: elevated load — amber warning before peak stress.
  final Color gridElevated;

  /// Grid status: peak stress active — red indicating load shedding.
  final Color gridPeakStress;

  /// Gateway online indicator.
  final Color gatewayOnline;

  /// Gateway offline indicator.
  final Color gatewayOffline;

  @override
  GridColors copyWith({
    Color? gridNormal,
    Color? gridElevated,
    Color? gridPeakStress,
    Color? gatewayOnline,
    Color? gatewayOffline,
  }) {
    return GridColors(
      gridNormal: gridNormal ?? this.gridNormal,
      gridElevated: gridElevated ?? this.gridElevated,
      gridPeakStress: gridPeakStress ?? this.gridPeakStress,
      gatewayOnline: gatewayOnline ?? this.gatewayOnline,
      gatewayOffline: gatewayOffline ?? this.gatewayOffline,
    );
  }

  @override
  GridColors lerp(ThemeExtension<GridColors>? other, double t) {
    if (other is! GridColors) return this;
    return GridColors(
      gridNormal: Color.lerp(gridNormal, other.gridNormal, t)!,
      gridElevated: Color.lerp(gridElevated, other.gridElevated, t)!,
      gridPeakStress: Color.lerp(gridPeakStress, other.gridPeakStress, t)!,
      gatewayOnline: Color.lerp(gatewayOnline, other.gatewayOnline, t)!,
      gatewayOffline: Color.lerp(gatewayOffline, other.gatewayOffline, t)!,
    );
  }
}

/// Default light-mode grid colors derived from [AppColors].
const gridColorsLight = GridColors(
  gridNormal: AppColors.gridNormal,
  gridElevated: AppColors.gridElevated,
  gridPeakStress: AppColors.gridPeakStress,
  gatewayOnline: AppColors.gatewayOnline,
  gatewayOffline: AppColors.gatewayOffline,
);

/// Builds the application [ThemeData] with the full [AppColors] palette
/// mapped into [ColorScheme] and the custom [GridColors] extension.
ThemeData buildAppTheme() {
  final colorScheme = ColorScheme.fromSeed(
    seedColor: AppColors.primary,
    brightness: Brightness.light,
    surface: AppColors.surface,
    onSurface: AppColors.textPrimary,
    onSurfaceVariant: AppColors.textSecondary,
    secondary: AppColors.secondary,
  );

  return ThemeData(
    colorScheme: colorScheme,
    useMaterial3: true,
    scaffoldBackgroundColor: AppColors.background,
    appBarTheme: AppBarTheme(
      backgroundColor: colorScheme.primary,
      foregroundColor: colorScheme.onPrimary,
    ),
    cardTheme: const CardThemeData(color: AppColors.surface),
    extensions: const <ThemeExtension<dynamic>>[gridColorsLight],
  );
}
