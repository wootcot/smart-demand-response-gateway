/// Data repository for the dashboard feature.
///
/// Provides HTTP-based communication with the Go backend server,
/// fetching grid status and deserializing JSON responses into domain models.
/// Uses the `http` package with a configurable timeout from core constants.
library;

import 'dart:convert';

import 'package:http/http.dart' as http;

import '../../../core/constants.dart';
import '../domain/models.dart';

/// Repository responsible for fetching grid data from the backend API.
///
/// Encapsulates HTTP transport details so that providers and presentation
/// layers depend only on domain models, not on network implementation.
class DashboardRepository {
  final http.Client _client;

  /// Creates a [DashboardRepository] with an optional HTTP [client].
  ///
  /// Accepts an injected client for testability; defaults to a standard
  /// [http.Client] when none is provided.
  DashboardRepository({http.Client? client})
      : _client = client ?? http.Client();

  /// Fetches the current grid status from the backend `/status` endpoint.
  ///
  /// Returns a [GridStatus] containing all connected gateways, total load,
  /// and peak stress state. Throws an [Exception] if the request fails or
  /// the server returns a non-200 status code.
  Future<GridStatus> fetchGridStatus() async {
    final uri = Uri.parse('$apiBaseUrl/status');

    final response = await _client
        .get(uri)
        .timeout(httpRequestTimeout);

    if (response.statusCode != 200) {
      throw Exception(
        'Failed to fetch grid status: HTTP ${response.statusCode}',
      );
    }

    final json = jsonDecode(response.body) as Map<String, dynamic>;
    return GridStatus.fromJson(json);
  }
}
