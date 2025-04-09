const WebSocket = require("ws");
const crypto = require("crypto");
const WavelengthModel = require("../models/wavelength");
const connectionManager = require("./connectionManager");
const {
  normalizeFrequency,
  generateSessionId,
  generateMessageId,
} = require("../utils/helpers");

/**
 * Support for registration of new wavelength
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Request data
 */
async function handleRegisterWavelength(ws, data) {
  const frequency = normalizeFrequency(data.frequency);
  const name = data.name || `Wavelength-${frequency}`;
  const isPasswordProtected = data.isPasswordProtected || false;
    const password = data.password || "";

  try {
    console.log(`Checking if frequency ${frequency} is available...`);

    // Memory check
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

    // Database check
    const existingWavelength = await WavelengthModel.findByFrequency(frequency);

    if (existingWavelength) {
      console.log(
        `Found existing wavelength ${frequency} in database, deleting it`
      );
      await WavelengthModel.delete(frequency);
    }

    const sessionId = generateSessionId("ws");

    await WavelengthModel.create(
      frequency,
      name,
      isPasswordProtected,
      sessionId,
      password
    );

    connectionManager.registerWavelength(
      ws,
      frequency,
      name,
      isPasswordProtected,
      password,
      sessionId
    );

    // Confirmation message
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
 * Support for joining wavelength
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Request data
 */
async function handleJoinWavelength(ws, data) {
  const frequency = normalizeFrequency(data.frequency);
  const password = data.password || "";

  const wavelength = connectionManager.activeWavelengths.get(frequency);

  if (!wavelength) {
    try {
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

  if (wavelength.isPasswordProtected) {
    console.log("Wavelength is password protected");
    if (!password) {
      ws.send(
          JSON.stringify({
            type: "join_result",
            success: false,
            error: "Password required"
          })
      );
      return;
    }

      const hashedInputPassword = crypto.createHash('sha256').update(password).digest('hex');
      const hashedStoredPassword = wavelength.password;

      console.log("Checking password hash:", hashedInputPassword);
      if (hashedInputPassword !== hashedStoredPassword) {
          ws.send(
              JSON.stringify({
                  type: "join_result",
                  success: false,
                  error: "Invalid password"
              })
          );
          return;
      }
  }

  const sessionId = generateSessionId("client");

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

  // Notify host about new client
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
 * Support for sending a text message
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Message data
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

  if (connectionManager.isMessageProcessed(frequency, messageId)) {
    return; // Ignore duplicates
  }

  connectionManager.markMessageProcessed(frequency, messageId);

  const isHost = ws.isHost;
  const sender = isHost
    ? "Host"
    : (ws.sessionId && ws.sessionId.substring(0, 8)) || "Client";

  console.log(
    `Message in wavelength ${frequency} from ${sender}: ${message} (ID: ${messageId})`
  );

  // Sending the message back to the sender for confirmation
  // TODO: probably remove this in production mode
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

  // Prepare a message to send to other clients
  const broadcastMessage = {
    type: "message",
    sender: sender,
    content: message,
    frequency,
    messageId,
    timestamp: new Date().toISOString(),
  };

  const broadcastStr = JSON.stringify(broadcastMessage);

  // Send to other clients (except the sender)
  wavelength.clients.forEach((client) => {
    if (client !== ws && client.readyState === WebSocket.OPEN) {
      client.send(broadcastStr);
    }
  });

  // Do not forget the host: if the sender is not the host, send to the host
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
 * File upload support
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - File data
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

  if (attachmentData && attachmentData.length > 15 * 1024 * 1024) {
    // ~10MB hard-stuck limit after base64 decoding (~15MB before decoding)
    // TODO: consider using streams for larger files or extend this limit
    ws.send(
      JSON.stringify({
        type: "error",
        error: "File size exceeds the maximum limit (10MB)",
      })
    );
    return;
  }

  if (connectionManager.isMessageProcessed(frequency, messageId)) {
    return;
  }

  connectionManager.markMessageProcessed(frequency, messageId);

  const isHost = ws.isHost;
  const sender = isHost
    ? "Host"
    : (ws.sessionId && ws.sessionId.substring(0, 8)) || "Client";

  console.log(
    `File received in wavelength ${frequency} from ${sender}: ${attachmentName} (${attachmentType}, ID: ${messageId})`
  );

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

  wavelength.clients.forEach((client) => {
    if (client !== ws && client.readyState === WebSocket.OPEN) {
      try {
        client.send(broadcastStr);
      } catch (e) {
        console.error("Error sending message to client:", e);
      }
    }
  });

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
 * Handler for leaving wavelength
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Request data
 */
async function handleLeaveWavelength(ws, data) {
  const frequency = normalizeFrequency(ws.frequency);

  if (!frequency) return;

  const wavelength = connectionManager.activeWavelengths.get(frequency);

  if (wavelength) {
    if (ws.isHost) {
      await closeWavelength(frequency, "Host left");
    } else {
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

  // Resetting WebSocket properties
  ws.frequency = null;
  ws.isHost = false;
}

/**
 * Handler for closing wavelength
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Request data
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
 * Closes wavelength and notifies all clients
 * @param {number} frequency - Frequency of the wavelength to close
 * @param {string} reason - Reason for closing the wavelength
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
    await WavelengthModel.delete(frequency);
    console.log(`Wavelength ${frequency} removed from database`);
  } catch (err) {
    console.error(`Error removing wavelength ${frequency} from database:`, err);
  }

  connectionManager.activeWavelengths.delete(frequency);
}

/**
 * Main handler for WebSocket messages
 * @param {WebSocket} ws - WebSocket connection
 * @param {string} message - Received message
 */
async function handleMessage(ws, message) {
  try {
    console.log("Received:", message.toString());
    const data = JSON.parse(message.toString());

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
