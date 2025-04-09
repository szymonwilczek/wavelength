const express = require("express");
const http = require("http");
require("dotenv").config();

const apiRouter = require("./routes/api");
const { initWebSocketServer } = require("./websocket/wsServer");
const { initDb } = require("./config/db");
const wavelengthService = require("./services/wavelengthService");
const frequencyTracker = require("./services/frequencyGapTracker");

const PORT = process.env.PORT || 3000;

const app = express();
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.use((req, res, next) => {
  console.log(`${req.method} ${req.url}`);
  next();
});

// CORS configuration
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

app.use(apiRouter);

app.use((req, res) => {
  res.status(404).json({ error: "Not found", status: "error" });
});

const server = http.createServer(app);

async function initializeApp() {
  try {
    console.log("Initializing database...");
    await initDb();

    console.log("Setting up wavelength service...");
    await wavelengthService.initializeDatabase();

    console.log("Initializing frequency tracker...");
    await frequencyTracker.initialize();

    console.log("Starting WebSocket server...");
    const wss = initWebSocketServer(server);

    server.listen(PORT, () => {
      console.log(`Server is running on port ${PORT}`);
      console.log(`REST API available at http://localhost:${PORT}/api`);
      console.log(`WebSocket server available at ws://localhost:${PORT}`);
    });

    setupServerShutdown(server, wss);
  } catch (error) {
    console.error("Failed to initialize application:", error);
    process.exit(1);
  }
}

function setupServerShutdown(server, wss) {
  const signals = ["SIGINT", "SIGTERM", "SIGQUIT"];

  signals.forEach((signal) => {
    process.on(signal, () => {
      console.log(`\nReceived ${signal}, shutting down gracefully...`);

      server.close(() => {
        console.log("HTTP server closed");
      });

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

initializeApp();
