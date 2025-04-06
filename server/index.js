/**
 * Główny plik serwera Wavelength
 */
const express = require("express");
const http = require("http");
const path = require("path");
require("dotenv").config();

// Importowanie komponentów
const apiRouter = require("./routes/api");
const { initWebSocketServer } = require("./websocket/wsServer");
const { initDb } = require("./config/db");
const wavelengthService = require("./services/wavelengthService");

// Konfiguracja podstawowa
const PORT = process.env.PORT || 3000;

// Inicjalizacja aplikacji Express
const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Logowanie zapytań
app.use((req, res, next) => {
  console.log(`${req.method} ${req.url}`);
  next();
});

// Konfiguracja CORS
app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header(
    "Access-Control-Allow-Headers",
    "Origin, X-Requested-With, Content-Type, Accept"
  );
  res.header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
  if (req.method === "OPTIONS") {
    return res.sendStatus(200);
  }
  next();
});

// Rejestracja routerów API
app.use(apiRouter);

// Obsługa statycznych plików (jeśli potrzebne w przyszłości)
// app.use(express.static(path.join(__dirname, 'public')));

// Obsługa błędu 404 dla nieobsługiwanych ścieżek
app.use((req, res) => {
  res.status(404).json({ error: "Not found", status: "error" });
});

// Inicjalizacja serwera HTTP
const server = http.createServer(app);

// Funkcja inicjalizująca aplikację
async function initializeApp() {
  try {
    // Inicjalizacja bazy danych
    console.log("Initializing database...");
    await initDb();

    // Czyszczenie i inicjalizacja tabeli wavelengths
    console.log("Setting up wavelength service...");
    await wavelengthService.initializeDatabase();

    // Inicjalizacja serwera WebSocket
    console.log("Starting WebSocket server...");
    const wss = initWebSocketServer(server);

    // Uruchomienie serwera HTTP
    server.listen(PORT, () => {
      console.log(`Server is running on port ${PORT}`);
      console.log(`REST API available at http://localhost:${PORT}/api`);
      console.log(`WebSocket server available at ws://localhost:${PORT}`);
    });

    // Obsługa zamknięcia serwera
    setupServerShutdown(server, wss);
  } catch (error) {
    console.error("Failed to initialize application:", error);
    process.exit(1);
  }
}

// Konfiguracja obsługi zamknięcia serwera
function setupServerShutdown(server, wss) {
  // Obsługa sygnałów zakończenia procesu
  const signals = ["SIGINT", "SIGTERM", "SIGQUIT"];

  signals.forEach((signal) => {
    process.on(signal, () => {
      console.log(`\nReceived ${signal}, shutting down gracefully...`);

      // Zamknięcie serwera HTTP
      server.close(() => {
        console.log("HTTP server closed");
      });

      // Zamknięcie wszystkich połączeń WebSocket
      wss &&
        wss.clients &&
        wss.clients.forEach((client) => {
          client.terminate();
        });

      setTimeout(() => {
        console.log("Exiting process");
        process.exit(0);
      }, 1000);
    });
  });
}

// Uruchomienie aplikacji
initializeApp();
