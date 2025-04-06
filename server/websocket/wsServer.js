const WebSocket = require("ws");
const { handleMessage } = require("./handlers");
const connectionManager = require("./connectionManager");

/**
 * Initializes and configures the WebSocket server
 * @param {http.Server} httpServer - HTTP server on which you want WebSocket to run
 * @returns {WebSocket.Server} - Configured WebSocket server
 */
function initWebSocketServer(httpServer) {
  console.log("Initializing WebSocket server...");

  const wss = new WebSocket.Server({
    server: httpServer,
    maxPayload: 20 * 1024 * 1024, // ~ 20MB limit for payload size
  });

  wss.on("connection", function connection(ws, req) {
    console.log("Client connected from:", req.socket.remoteAddress);

    ws.isAlive = true;

    ws.on("pong", () => {
      ws.isAlive = true;
    });

    // Message handling
    ws.on("message", async function incoming(message) {
      try {
        await handleMessage(ws, message);
      } catch (error) {
        console.error("Error processing message:", error);
        ws.send(
          JSON.stringify({
            type: "error",
            error: "Server error processing message",
          })
        );
      }
    });

    // Error handling
    ws.on("error", function onError(error) {
      console.error("WebSocket error:", error);
    });

    // Connection close handling
    ws.on("close", function onClose() {
      console.log("Client disconnected");
      connectionManager.handleDisconnect(ws);
    });

    // Heartbeat mechanism
    ws.send(
      JSON.stringify({
        type: "welcome",
        message: "Connected to Wavelength server",
        timestamp: new Date().toISOString(),
      })
    );
  });

  // Erorr handling for the server
  wss.on("error", function onServerError(error) {
    console.error("WebSocket server error:", error);
  });

  // Heartbeat mechanism
  connectionManager.startHeartbeat(wss);

  // Close heartbeat on server close
  wss.on("close", function onServerClose() {
    console.log("WebSocket server closing");
    connectionManager.stopHeartbeat();
  });

  console.log("WebSocket server initialized");

  return wss;
}

module.exports = { initWebSocketServer };
