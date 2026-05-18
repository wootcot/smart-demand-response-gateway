/// Entry point for the Nepal Grid Peak Load Controller dashboard application.
///
/// Initializes Flutter bindings, configures the Riverpod dependency injection
/// scope, and launches the MaterialApp with the grid-themed color scheme
/// routing to the main [DashboardScreen].
library;

import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'core/theme.dart';
import 'features/dashboard/presentation/dashboard_screen.dart';

/// Application entry point.
///
/// Async to allow any pre-launch initialization (e.g., shared preferences,
/// native plugin setup) before the widget tree is built. Wraps the entire
/// app in [ProviderScope] so all Riverpod providers are available throughout
/// the widget tree.
Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  runApp(const ProviderScope(child: NepalGridApp()));
}

/// Root application widget for the Nepal Grid Peak Load Controller.
///
/// Configures the [MaterialApp] with an explicit [ColorScheme] derived from
/// [AppColors.primary] and routes to the [DashboardScreen] as the home page.
class NepalGridApp extends StatelessWidget {
  const NepalGridApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Nepal Grid Controller',
      theme: buildAppTheme(),
      home: const DashboardScreen(),
    );
  }
}
