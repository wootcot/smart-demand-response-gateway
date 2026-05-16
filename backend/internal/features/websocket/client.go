// Package websocket implements the full-duplex WebSocket communication layer.
package websocket

import (
	"context"
	"encoding/json"

	"github.com/coder/websocket"
)

// Client represents a single connected ESP32 gateway over WebSocket.
// It wraps the underlying connection and provides typed message sending.
type Client struct {
	GatewayID string
	conn      *websocket.Conn
}

// NewClient wraps a WebSocket connection with gateway identity.
func NewClient(gatewayID string, conn *websocket.Conn) *Client {
	return &Client{
		GatewayID: gatewayID,
		conn:      conn,
	}
}

// SendPeakStress sends a peak-stress instruction to this gateway.
// Uses JSON text frames for compatibility with ESP32 WebSocket client.
func (c *Client) SendPeakStress(ctx context.Context, msg PeakStressMessage) error {
	data, err := json.Marshal(msg)
	if err != nil {
		return err
	}
	return c.conn.Write(ctx, websocket.MessageText, data)
}

// ReadMessage reads the next message from the gateway connection.
// Returns the raw bytes and message type for the caller to deserialize.
func (c *Client) ReadMessage(ctx context.Context) ([]byte, error) {
	_, data, err := c.conn.Read(ctx)
	return data, err
}

// Close gracefully closes the WebSocket connection.
func (c *Client) Close() error {
	return c.conn.Close(websocket.StatusNormalClosure, "server shutting down")
}
