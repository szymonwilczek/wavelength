const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const { MongoClient } = require("mongodb");

const PORT = process.env.PORT || 3000;
const MONGO_URI =
  process.env.MONGO_URI ||
  "mongodb+srv://wolfiksw:PrincePolo1@testcluster.taymr.mongodb.net/";
const DB_NAME = "wavelengthDB";

let messageCounter = 0;

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let db;
let activeWavelengths = new Map();

MongoClient.connect(MONGO_URI)
  .then((client) => {
    console.log("Connected to MongoDB");
    db = client.db(DB_NAME);

    return db.collection("activeWavelengths").deleteMany({});
  })
  .then(() => {
    console.log("Active wavelengths collection cleared");
  })
  .catch((err) => {
    console.error("MongoDB connection error:", err);
    process.exit(1);
  });

app.get("/health", (req, res) => {
  res.status(200).json({ status: "ok" });
});

app.get("/api/wavelengths", async (req, res) => {
  try {
    const wavelengths = await db
      .collection("activeWavelengths")
      .find(
        {},
        {
          projection: {
            frequency: 1,
            name: 1,
            isPasswordProtected: 1,
            createdAt: 1,
          },
        }
      )
      .toArray();

    res.json(wavelengths);
  } catch (err) {
    console.error("Error fetching wavelengths:", err);
    res.status(500).json({ error: "Server error" });
  }
});

wss.on("connection", function connection(ws, req) {
  console.log("Client connected:", req.socket.remoteAddress);

  ws.isAlive = true;

  ws.on("pong", () => {
    ws.isAlive = true;
  });

  ws.on("message", async function incoming(message) {
    try {
      console.log("Received:", message.toString());
      const data = JSON.parse(message.toString());

      if (data.type === "register_wavelength") {
        const frequency = data.frequency;
        const name = data.name || `Wavelength-${frequency}`;
        const isPasswordProtected = data.isPasswordProtected || false;

        try {
          console.log(`Checking if frequency ${frequency} is available...`);

          // Sprawdź w pamięci
          if (activeWavelengths.has(frequency)) {
            console.log(`Frequency ${frequency} already exists in memory`);
            ws.send(
              JSON.stringify({
                type: "register_result",
                success: false,
                error: "Frequency is already in use",
              })
            );
            return;
          }

          // Sprawdź w bazie danych
          const existingWavelength = await db
            .collection("activeWavelengths")
            .findOne({ frequency });

          if (existingWavelength) {
            console.log(
              `Found existing wavelength ${frequency} in database, deleting it`
            );
            await db.collection("activeWavelengths").deleteOne({ frequency });
          }

          // Utwórz nowy wpis
          const sessionId = `ws_${Date.now()}_${Math.floor(
            Math.random() * 1000
          )}`;

          await db.collection("activeWavelengths").insertOne({
            frequency,
            name,
            isPasswordProtected,
            hostSocketId: sessionId,
            createdAt: new Date(),
          });

          // Utwórz wpis w pamięci
          ws.frequency = frequency;
          ws.isHost = true;
          ws.sessionId = sessionId;

          activeWavelengths.set(frequency, {
            host: ws,
            clients: new Set(),
            name,
            isPasswordProtected,
            processedMessageIds: new Set(), // Dodajemy śledzenie przetworzonych wiadomości
          });

          ws.send(
            JSON.stringify({
              type: "register_result",
              success: true,
              sessionId,
              frequency,
            })
          );

          console.log(
            `Successfully registered wavelength ${frequency} with session ${sessionId}`
          );
        } catch (err) {
          console.error(`Error registering wavelength ${frequency}:`, err);

          ws.send(
            JSON.stringify({
              type: "register_result",
              success: false,
              error: "Server error, please try again",
            })
          );
        }
      } else if (data.type === "join_wavelength") {
        const frequency = data.frequency;
        const password = data.password || "";

        const wavelength = activeWavelengths.get(frequency);

        if (!wavelength) {
          try {
            const dbWavelength = await db
              .collection("activeWavelengths")
              .findOne({ frequency });

            if (!dbWavelength) {
              ws.send(
                JSON.stringify({
                  type: "join_result",
                  success: false,
                  error: "Wavelength does not exist",
                })
              );
              return;
            }

            // Wavelength exists in DB but not in memory (host is offline)
            ws.send(
              JSON.stringify({
                type: "join_result",
                success: false,
                error: "Wavelength host is offline",
              })
            );
            return;
          } catch (err) {
            console.error("Error checking wavelength in DB:", err);
            ws.send(
              JSON.stringify({
                type: "join_result",
                success: false,
                error: "Database error",
              })
            );
            return;
          }
        }

        if (wavelength.isPasswordProtected && password !== password) {
          ws.send(
            JSON.stringify({
              type: "join_result",
              success: false,
              error: "Invalid password",
            })
          );
          return;
        }

        ws.frequency = frequency;
        ws.isHost = false;
        ws.sessionId = `client_${Date.now()}_${Math.floor(
          Math.random() * 1000
        )}`;

        wavelength.clients.add(ws);

        ws.send(
          JSON.stringify({
            type: "join_result",
            success: true,
            frequency,
            name: wavelength.name,
          })
        );

        if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
          wavelength.host.send(
            JSON.stringify({
              type: "client_joined",
              clientId: ws.sessionId,
              frequency,
            })
          );
        }

        console.log(`Client ${ws.sessionId} joined wavelength ${frequency}`);
      } else if (data.type === "send_message") {
        const frequency = ws.frequency;
        const message = data.message;
        // Użyj dostarczonego messageId lub wygeneruj nowy
        const messageId =
          data.messageId ||
          `msg_${Date.now()}_${Math.floor(Math.random() * 10000)}`;

        if (!frequency) {
          ws.send(
            JSON.stringify({
              type: "error",
              error: "No frequency specified",
            })
          );
          return;
        }

        const wavelength = activeWavelengths.get(frequency);

        if (!wavelength) {
          ws.send(
            JSON.stringify({
              type: "error",
              error: "Wavelength does not exist",
            })
          );
          return;
        }

        const isHost = ws.isHost;
        const sender = isHost
          ? "Host"
          : (ws.sessionId && ws.sessionId.substring(0, 8)) || "Client";

        console.log(
          `Message in wavelength ${frequency} from ${sender}: ${message} (ID: ${messageId})`
        );

        // Zawsze wysyłaj wiadomość z powrotem do nadawcy dla potwierdzenia
        ws.send(
          JSON.stringify({
            type: "message",
            sender: "You",
            content: message,
            frequency,
            messageId, // Załącz messageId w potwierdzeniu
            timestamp: new Date().toISOString(),
            isSelf: true,
          })
        );

        // Przygotuj wiadomość do wysłania innym
        const broadcastMessage = {
          type: "message",
          sender: sender,
          content: message,
          frequency,
          messageId, // Załącz to samo messageId
          timestamp: new Date().toISOString(),
        };

        const broadcastStr = JSON.stringify(broadcastMessage);

        // Wyślij do pozostałych klientów (oprócz nadawcy)
        wavelength.clients.forEach((client) => {
          if (client !== ws && client.readyState === WebSocket.OPEN) {
            client.send(broadcastStr);
          }
        });

        // Jeśli nadawca nie jest hostem, wyślij do hosta
        if (
          !isHost &&
          wavelength.host &&
          wavelength.host !== ws &&
          wavelength.host.readyState === WebSocket.OPEN
        ) {
          wavelength.host.send(broadcastStr);
        }
      } else if (data.type === "leave_wavelength") {
        const frequency = ws.frequency;

        if (!frequency) return;

        const wavelength = activeWavelengths.get(frequency);

        if (wavelength) {
          if (ws.isHost) {
            // Host is leaving, close the wavelength
            await closeWavelength(frequency, "Host left");
          } else {
            // Client is leaving
            wavelength.clients.delete(ws);

            if (
              wavelength.host &&
              wavelength.host.readyState === WebSocket.OPEN
            ) {
              wavelength.host.send(
                JSON.stringify({
                  type: "client_left",
                  clientId: ws.sessionId,
                  frequency,
                })
              );
            }

            console.log(`Client ${ws.sessionId} left wavelength ${frequency}`);
          }
        }

        // Reset socket state
        ws.frequency = null;
        ws.isHost = false;
      } else if (data.type === "close_wavelength") {
        const frequency = ws.frequency;

        if (!frequency) return;

        if (!ws.isHost) {
          ws.send(
            JSON.stringify({
              type: "error",
              error: "Only host can close wavelength",
            })
          );
          return;
        }

        await closeWavelength(frequency, "Host closed the wavelength");

        // Wysyłamy potwierdzenie użytkownikowi przed zamknięciem
        ws.send(
          JSON.stringify({
            type: "close_confirmed",
            frequency,
          })
        );

        // Reset socket state
        ws.frequency = null;
        ws.isHost = false;

        console.log(`Host closed wavelength ${frequency}`);
      }
    } catch (e) {
      console.error("Error processing message:", e);
      ws.send(
        JSON.stringify({
          type: "error",
          error: "Invalid message format",
        })
      );
    }
  });

  ws.on("close", function () {
    console.log(`Client disconnected: ${ws.sessionId || "unknown"}`);

    if (ws.frequency) {
      const frequency = ws.frequency;
      const wavelength = activeWavelengths.get(frequency);

      if (wavelength) {
        if (ws.isHost) {
          // Natychmiastowo usuń z pamięci aby zapobiec dostępowi przez nowe połączenia
          activeWavelengths.delete(frequency);

          // Asynchronicznie powiadom klientów i oczyść bazę danych
          closeWavelength(frequency, "Host disconnected");
        } else {
          // Client disconnected
          wavelength.clients.delete(ws);

          if (
            wavelength.host &&
            wavelength.host.readyState === WebSocket.OPEN
          ) {
            wavelength.host.send(
              JSON.stringify({
                type: "client_left",
                clientId: ws.sessionId,
                frequency,
              })
            );
          }
        }
      }
    }
  });

  ws.on("error", function (err) {
    console.error("WebSocket error:", err);
  });
});

async function closeWavelength(frequency, reason) {
  console.log(`Closing wavelength ${frequency}: ${reason}`);

  const wavelength = activeWavelengths.get(frequency);

  if (!wavelength) {
    console.log(`Wavelength ${frequency} not found in memory`);
    return;
  }

  const closeMessage = JSON.stringify({
    type: "wavelength_closed",
    frequency,
    reason,
  });

  // Notify all clients
  wavelength.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      try {
        client.send(closeMessage);
      } catch (e) {
        console.error("Error sending close message to client:", e);
      }
    }
  });

  try {
    // Remove from database
    await db.collection("activeWavelengths").deleteOne({ frequency });
    console.log(`Wavelength ${frequency} removed from database`);
  } catch (err) {
    console.error(`Error removing wavelength ${frequency} from database:`, err);
  }

  // Remove from memory
  activeWavelengths.delete(frequency);
}

// Ping clients to keep connections alive
const interval = setInterval(function ping() {
  wss.clients.forEach(function each(ws) {
    if (ws.isAlive === false) {
      console.log("Terminating inactive connection");
      return ws.terminate();
    }

    ws.isAlive = false;
    ws.ping();
  });
}, 30000);

wss.on("close", function close() {
  clearInterval(interval);
});

server.listen(PORT, () => {
  console.log(`Wavelength relay server running on port ${PORT}`);
});

process.on("SIGINT", async () => {
  console.log("Shutting down server...");

  if (db) {
    try {
      await db.collection("activeWavelengths").deleteMany({});
      console.log("Cleared active wavelengths");
    } catch (err) {
      console.error("Error cleaning database on shutdown:", err);
    }
  }

  process.exit(0);
});
