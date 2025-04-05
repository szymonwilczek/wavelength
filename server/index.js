const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const { Pool } = require("pg");

const PORT = process.env.PORT || 3000;

const connectionString =
  "postgresql://u_f67b73d1_c584_40b2_a189_3cf401949c75:NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2@pg.rapidapp.io:5433/db_f67b73d1_c584_40b2_a189_3cf401949c75?ssl=true&sslmode=no-verify&application_name=rapidapp_nodejs";

// Inicjalizacja puli połączeń
const pool = new Pool({
  connectionString,
  ssl: {
    rejectUnauthorized: false, // Wymagane dla "sslmode=no-verify"
  },
});

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let activeWavelengths = new Map();
let lastRegisteredFrequency = 130.0;

async function initDatabase() {
  try {
    // Sprawdź połączenie
    const res = await pool.query("SELECT NOW()");
    console.log("Connected to PostgreSQL:", res.rows[0].now);

    // Upewnij się, że tabela istnieje (tylko przy pierwszym uruchomieniu)
    await pool.query(`
      CREATE TABLE IF NOT EXISTS active_wavelengths (
        id SERIAL PRIMARY KEY,
        frequency NUMERIC(12, 1) UNIQUE NOT NULL,
        name VARCHAR(255) NOT NULL,
        is_password_protected BOOLEAN DEFAULT FALSE,
        host_socket_id VARCHAR(255) NOT NULL,
        created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
      );
    `);

    // Utwórz indeksy, jeśli nie istnieją
    await pool.query(`
      CREATE INDEX IF NOT EXISTS idx_active_wavelengths_frequency
      ON active_wavelengths(frequency);
    `);

    // Wyczyść tabelę active_wavelengths
    await pool.query("DELETE FROM active_wavelengths");
    console.log("Active wavelengths table cleared");
  } catch (err) {
    console.error("PostgreSQL connection error:", err);
    process.exit(1);
  }
}

initDatabase();

app.get("/health", (req, res) => {
  res.status(200).json({ status: "ok" });
});

app.get("/api/next-available-frequency", async (req, res) => {
  try {
    // Znajdź następną dostępną częstotliwość zaczynając od ostatniej zarejestrowanej
    let nextFreq = lastRegisteredFrequency;
    const increment = 0.1; // Zwiększamy o 0.1 Hz
    let foundAvailable = false;
    let safetyCounter = 0; // Zapobiega nieskończonym pętlom

    console.log(
      `Looking for next available frequency starting from ${nextFreq} Hz`
    );

    // Sprawdź częstotliwości w pamięci
    while (!foundAvailable && safetyCounter < 1000) {
      nextFreq += increment;
      nextFreq = normalizeFrequency(nextFreq); // Zaokrąglenie do 1 miejsca po przecinku

      // Jeśli nie ma takiej częstotliwości w mapie aktywnych
      if (!activeWavelengths.has(nextFreq)) {
        // Sprawdź dodatkowo w bazie danych (na wszelki wypadek)
        const existingResult = await pool.query(
          "SELECT frequency FROM active_wavelengths WHERE frequency = $1",
          [nextFreq]
        );

        if (existingResult.rows.length === 0) {
          foundAvailable = true;
          break;
        }
      }

      safetyCounter++;
    }

    // Jeśli znaleźliśmy częstotliwość, zwróć ją
    if (foundAvailable) {
      console.log(`Found available frequency: ${nextFreq} Hz`);
      res.json({ frequency: nextFreq });
    } else {
      // Jeśli nie znaleźliśmy, zwróć błąd
      console.log("Could not find available frequency within range");
      res.status(404).json({ error: "No available frequency found" });
    }
  } catch (err) {
    console.error("Error finding available frequency:", err);
    res.status(500).json({ error: "Server error" });
  }
});

function normalizeFrequency(frequency) {
  // Konwersja do liczby jeśli to string
  const numFreq =
    typeof frequency === "string" ? parseFloat(frequency) : frequency;
  // Zaokrąglenie do 1 miejsca po przecinku i konwersja z powrotem do liczby
  return parseFloat(numFreq.toFixed(1));
}

app.get("/api/wavelengths", async (req, res) => {
  try {
    const result = await pool.query(`
      SELECT 
        frequency, 
        name, 
        is_password_protected AS "isPasswordProtected", 
        created_at AS "createdAt" 
      FROM active_wavelengths
    `);

    res.json(result.rows);
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
        const frequency = normalizeFrequency(data.frequency);
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
          const existingResult = await pool.query(
            "SELECT frequency FROM active_wavelengths WHERE frequency = $1",
            [frequency]
          );

          if (existingResult.rows.length > 0) {
            console.log(
              `Found existing wavelength ${frequency} in database, deleting it`
            );
            await pool.query(
              "DELETE FROM active_wavelengths WHERE frequency = $1",
              [frequency]
            );
          }

          // Utwórz nowy wpis
          const sessionId = `ws_${Date.now()}_${Math.floor(
            Math.random() * 1000
          )}`;

          await pool.query(
            "INSERT INTO active_wavelengths (frequency, name, is_password_protected, host_socket_id, created_at) VALUES ($1, $2, $3, $4, $5)",
            [frequency, name, isPasswordProtected, sessionId, new Date()]
          );

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

          if (frequency > lastRegisteredFrequency) {
            lastRegisteredFrequency = frequency;
            console.log(`Updated last registered frequency to ${frequency} Hz`);
          }

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
        const frequency = normalizeFrequency(data.frequency);
        const password = data.password || "";

        const wavelength = activeWavelengths.get(frequency);

        if (!wavelength) {
          try {
            const dbWavelengthResult = await pool.query(
              "SELECT * FROM active_wavelengths WHERE frequency = $1",
              [frequency]
            );
            const dbWavelength = dbWavelengthResult.rows[0];

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
        const frequency = normalizeFrequency(ws.frequency);
        const message = data.content || data.message;
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
      } else if (data.type === "send_file") {
        const frequency = normalizeFrequency(ws.frequency);
        const messageId =
          data.messageId ||
          `file_${Date.now()}_${Math.floor(Math.random() * 10000)}`;
        const hasAttachment = true; // Zawsze true dla plików
        const attachmentType = data.attachmentType;
        const attachmentMimeType = data.attachmentMimeType;
        const attachmentName = data.attachmentName;
        const attachmentData = data.attachmentData;
        const senderId = data.senderId;
        const timestamp = data.timestamp || Date.now();

        if (!frequency) {
          ws.send(
            JSON.stringify({
              type: "error",
              error: "You are not connected to any wavelength",
            })
          );
          return;
        }

        const wavelength = activeWavelengths.get(frequency);

        if (!wavelength) {
          ws.send(
            JSON.stringify({
              type: "error",
              error: "Wavelength not found",
            })
          );
          return;
        }

        // Sprawdź rozmiar danych
        if (attachmentData && attachmentData.length > 15 * 1024 * 1024) {
          // Limit ~10MB po dekodowaniu base64
          ws.send(
            JSON.stringify({
              type: "error",
              error: "File size exceeds the maximum limit (10MB)",
            })
          );
          return;
        }

        const isHost = ws.isHost;
        const sender = isHost
          ? "Host"
          : (ws.sessionId && ws.sessionId.substring(0, 8)) || "Client";

        console.log(
          `File received in wavelength ${frequency} from ${sender}: ${attachmentName} (${attachmentType}, ID: ${messageId})`
        );

        // Jeśli wiadomość była już przetworzona (ma ID), zignoruj
        if (
          wavelength.processedMessageIds &&
          wavelength.processedMessageIds.has(messageId)
        ) {
          console.log(`Ignoring duplicate message ID: ${messageId}`);
          return;
        }

        // Dodaj ID wiadomości do przetworzonych
        if (wavelength.processedMessageIds) {
          wavelength.processedMessageIds.add(messageId);

          // Ogranicz rozmiar zestawu przetworzonych ID
          if (wavelength.processedMessageIds.size > 1000) {
            const iterator = wavelength.processedMessageIds.values();
            wavelength.processedMessageIds.delete(iterator.next().value);
          }
        }

        // Zawsze wysyłaj potwierdzenie do nadawcy
        ws.send(
          JSON.stringify({
            type: "message",
            hasAttachment: true,
            attachmentType: attachmentType,
            attachmentMimeType: attachmentMimeType,
            attachmentName: attachmentName,
            attachmentData: attachmentData,
            frequency: frequency,
            messageId: messageId,
            timestamp: new Date(timestamp).toISOString(),
            isSelf: true,
            senderId: senderId,
          })
        );

        // Przygotuj wiadomość do wysłania innym
        const broadcastMessage = {
          type: "message",
          hasAttachment: true,
          attachmentType: attachmentType,
          attachmentMimeType: attachmentMimeType,
          attachmentName: attachmentName,
          attachmentData: attachmentData,
          frequency: frequency,
          messageId: messageId,
          timestamp: new Date(timestamp).toISOString(),
          sender: sender,
          senderId: senderId,
        };

        const broadcastStr = JSON.stringify(broadcastMessage);

        // Wyślij do pozostałych klientów (oprócz nadawcy)
        wavelength.clients.forEach((client) => {
          if (client !== ws && client.readyState === WebSocket.OPEN) {
            try {
              client.send(broadcastStr);
            } catch (e) {
              console.error("Error sending message to client:", e);
            }
          }
        });

        // Jeśli nadawca nie jest hostem, wyślij do hosta
        if (
          !isHost &&
          wavelength.host &&
          wavelength.host !== ws &&
          wavelength.host.readyState === WebSocket.OPEN
        ) {
          try {
            wavelength.host.send(broadcastStr);
          } catch (e) {
            console.error("Error sending message to host:", e);
          }
        }
      } else if (data.type === "leave_wavelength") {
        const frequency = normalizeFrequency(ws.frequency);

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
        const frequency = normalizeFrequency(ws.frequency);

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
              error: "Wavelength not found",
            })
          );
          return;
        }

        if (ws !== wavelength.host) {
          ws.send(
            JSON.stringify({
              type: "error",
              error: "Only host can close wavelength",
            })
          );
          return;
        }

        // Zamknij wavelength
        closeWavelength(frequency, "Host closed the wavelength");

        ws.send(
          JSON.stringify({
            type: "close_result",
            success: true,
            frequency,
          })
        );
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
    await pool.query("DELETE FROM active_wavelengths WHERE frequency = $1", [
      frequency,
    ]);
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

  try {
    await pool.query("DELETE FROM active_wavelengths");
    console.log("Cleared active wavelengths");
    await pool.end(); // Zamknij połączenie z bazą danych przed zakończeniem
  } catch (err) {
    console.error("Error cleaning database on shutdown:", err);
  }

  process.exit(0);
});
