// Package shared provides common types and contracts used across feature packages.
// It serves as the dependency root for cross-feature type sharing, ensuring features
// remain decoupled from each other while sharing a consistent vocabulary.
package shared

import "time"

// GatewayID is a typed string for gateway identification across the system.
// Using a type alias allows features to reference a semantic type rather than
// raw strings, improving code readability and enabling future migration to a
// distinct type if validation logic is needed.
type GatewayID = string

// NowUTC returns the current time in UTC. This indirection allows test code to
// verify timestamp behavior without coupling to wall-clock time directly in
// production paths.
func NowUTC() time.Time {
	return time.Now().UTC()
}

// ParseTimestamp parses an RFC3339-formatted timestamp string into a time.Time value.
// The gateway firmware and dashboard both use RFC3339 as the wire format for timestamps.
func ParseTimestamp(s string) (time.Time, error) {
	return time.Parse(time.RFC3339, s)
}

// FormatTimestamp formats a time.Time value into an RFC3339 string suitable for
// JSON serialization in API responses.
func FormatTimestamp(t time.Time) string {
	return t.UTC().Format(time.RFC3339)
}
