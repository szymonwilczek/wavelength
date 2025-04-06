/**
 * Inicjalizacja i konfiguracja serwera WebSocket
 */
const WebSocket = require("ws");
const { handleMessage } = require("./handlers");
const connectionManager = require("./connectionManager");

/**
 * Inicjalizuje i konfiguruje serwer WebSocket
 * @param {http.Server} httpServer - Serwer HTTP, na którym ma działać WebSocket
 * @returns {WebSocket.Server} - Skonfigurowany serwer WebSocket
 */
function initWebSocketServer(httpServer) {
  console.log("Initializing WebSocket server...");

  const wss = new WebSocket.Server({
    server: httpServer,
    // Można dodać dodatkowe opcje, np. maxPayload
    maxPayload: 20 * 1024 * 1024, // 20MB limit na wiadomości
  });

  // Obsługa nowych połączeń
  wss.on("connection", function connection(ws, req) {
    console.log("Client connected from:", req.socket.remoteAddress);

    // Inicjalizacja stanu klienta
    ws.isAlive = true;

    // Obsługa ping/pong do wykrywania rozłączonych klientów
    ws.on("pong", () => {
      ws.isAlive = true;
    });

    // Obsługa wiadomości
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

    // Obsługa błędów
    ws.on("error", function onError(error) {
      console.error("WebSocket error:", error);
    });

    // Obsługa zamknięcia połączenia
    ws.on("close", function onClose() {
      console.log("Client disconnected");
      connectionManager.handleDisconnect(ws);
    });

    // Wyślij wiadomość powitalną
    ws.send(
      JSON.stringify({
        type: "welcome",
        message: "Connected to Wavelength server",
        timestamp: new Date().toISOString(),
      })
    );
  });

  // Obsługa błędów na poziomie serwera
  wss.on("error", function onServerError(error) {
    console.error("WebSocket server error:", error);
  });

  // Uruchom mechanizm heartbeat do wykrywania rozłączonych klientów
  connectionManager.startHeartbeat(wss);

  // Zamknięcie heartbeat przy zamykaniu serwera
  wss.on("close", function onServerClose() {
    console.log("WebSocket server closing");
    connectionManager.stopHeartbeat();
  });

  console.log("WebSocket server initialized");

  return wss;
}

module.exports = { initWebSocketServer };
