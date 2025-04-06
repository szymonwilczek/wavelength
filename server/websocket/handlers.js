const WebSocket = require("ws");
const WavelengthModel = require("../models/wavelength");
const connectionManager = require("./connectionManager");
const {
  normalizeFrequency,
  generateSessionId,
  generateMessageId,
} = require("../utils/helpers");

/**
 * Obsługa rejestracji nowego wavelength
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {Object} data - Dane żądania
 */
async function handleRegisterWavelength(ws, data) {
  const frequency = normalizeFrequency(data.frequency);
  const name = data.name || `Wavelength-${frequency}`;
  const isPasswordProtected = data.isPasswordProtected || false;

  try {
    console.log(`Checking if frequency ${frequency} is available...`);

    // Sprawdź w pamięci
    if (connectionManager.activeWavelengths.has(frequency)) {
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
    const existingWavelength = await WavelengthModel.findByFrequency(frequency);

    if (existingWavelength) {
      console.log(
        `Found existing wavelength ${frequency} in database, deleting it`
      );
      await WavelengthModel.delete(frequency);
    }

    // Utwórz nowy wpis
    const sessionId = generateSessionId("ws");

    // Zapisz do bazy danych
    await WavelengthModel.create(
      frequency,
      name,
      isPasswordProtected,
      sessionId
    );

    // Zarejestruj w managerze połączeń
    connectionManager.registerWavelength(
      ws,
      frequency,
      name,
      isPasswordProtected,
      sessionId
    );

    // Wyślij potwierdzenie
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
}

/**
 * Obsługa dołączania do wavelength
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {Object} data - Dane żądania
 */
async function handleJoinWavelength(ws, data) {
  const frequency = normalizeFrequency(data.frequency);
  const password = data.password || "";

  const wavelength = connectionManager.activeWavelengths.get(frequency);

  if (!wavelength) {
    try {
      // Sprawdź w bazie danych
      const dbWavelength = await WavelengthModel.findByFrequency(frequency);

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

      // Wavelength istnieje w bazie, ale host jest offline
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

  // Sprawdź hasło
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

  // Generuj ID sesji dla klienta
  const sessionId = generateSessionId("client");

  // Dodaj klienta do wavelength
  connectionManager.addClientToWavelength(ws, frequency, sessionId);

  ws.send(
    JSON.stringify({
      type: "join_result",
      success: true,
      frequency,
      name: wavelength.name,
      sessionId,
    })
  );

  // Powiadom hosta o dołączeniu klienta
  if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
    wavelength.host.send(
      JSON.stringify({
        type: "client_joined",
        clientId: sessionId,
        frequency,
      })
    );
  }

  console.log(`Client ${sessionId} joined wavelength ${frequency}`);
}

/**
 * Obsługa wysyłania wiadomości tekstowej
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {Object} data - Dane wiadomości
 */
function handleSendMessage(ws, data) {
  const frequency = normalizeFrequency(ws.frequency);
  const message = data.content || data.message;
  const messageId = data.messageId || generateMessageId("msg");

  if (!frequency) {
    ws.send(
      JSON.stringify({
        type: "error",
        error: "No frequency specified",
      })
    );
    return;
  }

  const wavelength = connectionManager.activeWavelengths.get(frequency);

  if (!wavelength) {
    ws.send(
      JSON.stringify({
        type: "error",
        error: "Wavelength does not exist",
      })
    );
    return;
  }

  // Sprawdź, czy wiadomość była już przetworzona
  if (connectionManager.isMessageProcessed(frequency, messageId)) {
    return; // Ignoruj duplikaty
  }

  // Oznacz wiadomość jako przetworzoną
  connectionManager.markMessageProcessed(frequency, messageId);

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
      messageId,
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
    messageId,
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
}

/**
 * Obsługa wysyłania plików
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {Object} data - Dane pliku
 */
function handleSendFile(ws, data) {
  const frequency = normalizeFrequency(ws.frequency);
  const messageId = data.messageId || generateMessageId("file");
  const attachmentType = data.attachmentType;
  const attachmentMimeType = data.attachmentMimeType;
  const attachmentName = data.attachmentName;
  const attachmentData = data.attachmentData;
  const senderId = data.senderId || ws.sessionId;
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

  const wavelength = connectionManager.activeWavelengths.get(frequency);

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

  // Sprawdź, czy wiadomość była już przetworzona
  if (connectionManager.isMessageProcessed(frequency, messageId)) {
    return; // Ignoruj duplikaty
  }

  // Oznacz wiadomość jako przetworzoną
  connectionManager.markMessageProcessed(frequency, messageId);

  const isHost = ws.isHost;
  const sender = isHost
    ? "Host"
    : (ws.sessionId && ws.sessionId.substring(0, 8)) || "Client";

  console.log(
    `File received in wavelength ${frequency} from ${sender}: ${attachmentName} (${attachmentType}, ID: ${messageId})`
  );

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
}

/**
 * Obsługa opuszczania wavelength
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {Object} data - Dane żądania
 */
async function handleLeaveWavelength(ws, data) {
  const frequency = normalizeFrequency(ws.frequency);

  if (!frequency) return;

  const wavelength = connectionManager.activeWavelengths.get(frequency);

  if (wavelength) {
    if (ws.isHost) {
      // Host opuszcza wavelength
      await closeWavelength(frequency, "Host left");
    } else {
      // Klient opuszcza wavelength
      wavelength.clients.delete(ws);

      if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
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

  // Zresetuj stan socketu
  ws.frequency = null;
  ws.isHost = false;
}

/**
 * Obsługa zamykania wavelength
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {Object} data - Dane żądania
 */
async function handleCloseWavelength(ws, data) {
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

  const wavelength = connectionManager.activeWavelengths.get(frequency);

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

  await closeWavelength(frequency, "Host closed the wavelength");

  ws.send(
    JSON.stringify({
      type: "close_result",
      success: true,
      frequency,
    })
  );
}

/**
 * Zamyka wavelength i powiadamia wszystkich klientów
 * @param {number} frequency - Częstotliwość wavelength do zamknięcia
 * @param {string} reason - Przyczyna zamknięcia
 */
async function closeWavelength(frequency, reason) {
  console.log(`Closing wavelength ${frequency}: ${reason}`);

  const wavelength = connectionManager.activeWavelengths.get(frequency);

  if (!wavelength) {
    console.log(`Wavelength ${frequency} not found in memory`);
    return;
  }

  const closeMessage = JSON.stringify({
    type: "wavelength_closed",
    frequency,
    reason,
  });

  // Powiadom wszystkich klientów
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
    // Usuń z bazy danych
    await WavelengthModel.delete(frequency);
    console.log(`Wavelength ${frequency} removed from database`);
  } catch (err) {
    console.error(`Error removing wavelength ${frequency} from database:`, err);
  }

  // Usuń z pamięci
  connectionManager.activeWavelengths.delete(frequency);
}

/**
 * Główny handler dla wiadomości WebSocket
 * @param {WebSocket} ws - Połączenie WebSocket
 * @param {string} message - Otrzymana wiadomość
 */
async function handleMessage(ws, message) {
  try {
    console.log("Received:", message.toString());
    const data = JSON.parse(message.toString());

    // Wybierz odpowiedni handler na podstawie typu wiadomości
    switch (data.type) {
      case "register_wavelength":
        await handleRegisterWavelength(ws, data);
        break;
      case "join_wavelength":
        await handleJoinWavelength(ws, data);
        break;
      case "send_message":
        handleSendMessage(ws, data);
        break;
      case "send_file":
        handleSendFile(ws, data);
        break;
      case "leave_wavelength":
        await handleLeaveWavelength(ws, data);
        break;
      case "close_wavelength":
        await handleCloseWavelength(ws, data);
        break;
      default:
        ws.send(
          JSON.stringify({
            type: "error",
            error: "Unknown message type",
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
}

module.exports = {
  handleMessage,
  handleRegisterWavelength,
  handleJoinWavelength,
  handleSendMessage,
  handleSendFile,
  handleLeaveWavelength,
  handleCloseWavelength,
  closeWavelength,
};
