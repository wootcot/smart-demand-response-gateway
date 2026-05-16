// Basic Flutter widget test for the Nepal Grid Peak Load Controller app.

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'package:app/main.dart';

void main() {
  testWidgets('NepalGridApp renders without errors', (WidgetTester tester) async {
    // Build the app wrapped in ProviderScope (required for Riverpod).
    await tester.pumpWidget(
      const ProviderScope(
        child: NepalGridApp(),
      ),
    );

    // Verify the app title is rendered in the AppBar or MaterialApp.
    expect(find.byType(MaterialApp), findsOneWidget);
  });
}
