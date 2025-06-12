const WebSocket = require("ws");
const crypto = require("crypto");
const WavelengthModel = require("../models/wavelength"); 
const connectionManager = require("./connectionManager"); 
const {
    normalizeFrequency, 
    generateSessionId,
    generateMessageId,
    isValidFrequency 
} = require("../utils/helpers");
const wavelengthService = require("../services/wavelengthService"); 

/**
 * Support for registration of new wavelength
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Request data (data.frequency can be number or string)
 */
async function handleRegisterWavelength(ws, data) {
    let rawFrequency = data.frequency;
    if (!isValidFrequency(rawFrequency)) {
        console.error("Invalid frequency format for registration:", rawFrequency);
        ws.send(JSON.stringify({ type: "register_result", success: false, error: "Invalid frequency format. Must be a positive number." }));
        return;
    }
    const frequency = normalizeFrequency(rawFrequency); 

    const name = data.name || `Wavelength-${frequency}`;
    const isPasswordProtected = data.isPasswordProtected || false;
    const password = data.password || null; 

    try {
        console.log(`Handler: Attempting to register frequency ${frequency}...`);

        const result = await wavelengthService.registerWavelength(
            ws,
            rawFrequency, 
            name,
            isPasswordProtected,
            password
        );

        ws.send(JSON.stringify({
            type: "register_result",
            ...result 
        }));

        if (result.success) {
            console.log(`Handler: Successfully registered wavelength ${result.frequency} with session ${result.sessionId}`);
        } else {
            console.log(`Handler: Failed to register wavelength ${frequency}: ${result.error}`);
        }

    } catch (err) {
        console.error(`Handler: Error registering wavelength ${frequency}:`, err);
        ws.send(
            JSON.stringify({
                type: "register_result",
                success: false,
                error: "Server error during registration, please try again",
            })
        );
    }
}

/**
 * Support for joining wavelength
 * @param {WebSocket} ws - WebSocket connection
 * @param {Object} data - Request data (data.frequency can be number or string)
 */
async function handleJoinWavelength(ws, data) {
    let rawFrequency = data.frequency;
    if (!isValidFrequency(rawFrequency)) {
        console.error("Invalid frequency format for join:", rawFrequency);
        ws.send(JSON.stringify({ type: "join_result", success: false, error: "Invalid frequency format. Must be a positive number." }));
        return;
    }
    const frequency = normalizeFrequency(rawFrequency); 
    const password = data.password || ""; 

    try {
        console.log(`Handler: Attempting to join frequency ${frequency}...`);

        // 1. Check ConnectionManager (memory) first
        const wavelengthInfo = connectionManager.activeWavelengths.get(frequency);

        if (!wavelengthInfo) {
            // 2. If not in memory, check DB to distinguish between offline host and non-existent wavelength
            console.log(`Handler: Wavelength ${frequency} not in memory, checking DB.`);
            const dbWavelength = await WavelengthModel.findByFrequency(frequency);
            const errorMsg = dbWavelength ? "Wavelength host is offline" : "Wavelength does not exist";
            ws.send(JSON.stringify({ type: "join_result", success: false, error: errorMsg }));
            console.log(`Handler: Join failed for ${frequency}: ${errorMsg}`);
            return;
        }

        // 3. Wavelength exists in memory, check password if needed
        if (wavelengthInfo.isPasswordProtected) {
            console.log(`Handler: Wavelength ${frequency} is password protected. Verifying password.`);
            if (!password) {
                ws.send(JSON.stringify({ type: "join_result", success: false, error: "Password required" }));
                return;
            }

            const hashedInputPassword = crypto.createHash('sha256').update(password).digest('hex');
            const hashedStoredPassword = wavelengthInfo.password; 

            if (hashedInputPassword !== hashedStoredPassword) {
                ws.send(JSON.stringify({ type: "join_result", success: false, error: "Invalid password" }));
                console.log(`Handler: Invalid password attempt for ${frequency}`);
                return;
            }
            console.log(`Handler: Password verified for ${frequency}`);
        }

        // 4. Password OK or not required, add client via ConnectionManager
        const sessionId = generateSessionId("client");
        const joinResult = connectionManager.addClientToWavelength(ws, frequency, sessionId);

        if (joinResult) {
            ws.send(
                JSON.stringify({
                    type: "join_result",
                    success: true,
                    frequency: joinResult.frequency, 
                    name: joinResult.name,
                    sessionId: joinResult.sessionId,
                })
            );

            // 5. Notify host about new client
            if (wavelengthInfo.host && wavelengthInfo.host.readyState === WebSocket.OPEN) {
                wavelengthInfo.host.send(
                    JSON.stringify({
                        type: "client_joined",
                        clientId: sessionId,
                        frequency: frequency, 
                    })
                );
            }
            console.log(`Handler: Client ${sessionId} joined wavelength ${frequency}`);
        } else {
            console.error(`Handler: Failed to add client ${sessionId} to wavelength ${frequency} unexpectedly.`);
            throw new Error("Internal error adding client to wavelength.");
        }

    } catch (err) {
        console.error(`Handler: Error joining wavelength ${frequency}:`, err);
        ws.send(
            JSON.stringify({
                type: "join_result",
                success: false,
                error: "Server error during join, please try again",
            })
        );
    }
}


/**
 * Support for sending a text message
 * @param {WebSocket} ws - WebSocket connection (ws.frequency is string "XXX.X")
 * @param {Object} data - Message data
 */
function handleSendMessage(ws, data) {
    const frequency = ws.frequency; 
    const messageContent = data.content || data.message; 
    const messageId = data.messageId || generateMessageId("msg");

    if (!frequency) {
        console.error(`Handler: send_message attempt from client ${ws.sessionId || 'unknown'} not connected to a frequency.`);
        ws.send(JSON.stringify({ type: "error", error: "Cannot send message: Not connected to a frequency" }));
        return;
    }
    if (!messageContent) {
        console.warn(`Handler: send_message received empty message content on ${frequency} from ${ws.sessionId || 'unknown'}.`);
        return;
    }


    const wavelength = connectionManager.activeWavelengths.get(frequency); 

    if (!wavelength) {
        console.error(`Handler: Wavelength ${frequency} not found for send_message from ${ws.sessionId || 'unknown'}.`);
        ws.send(JSON.stringify({ type: "error", error: "Wavelength does not exist or host is offline" }));
        return;
    }

    if (connectionManager.isMessageProcessed(frequency, messageId)) {
        console.log(`Handler: Ignoring duplicate message ${messageId} on ${frequency}`);
        return; 
    }
    connectionManager.markMessageProcessed(frequency, messageId);

    const isHost = ws.isHost;
    const sender = isHost ? "Host" : (ws.sessionId ? ws.sessionId.substring(0, 8) : "Client"); 

    console.log(`Handler: Message on ${frequency} from ${sender} (ID: ${messageId}): ${messageContent}`);

    ws.send(
        JSON.stringify({
            type: "message",
            sender: "You", 
            content: messageContent,
            frequency: frequency, 
            messageId,
            timestamp: new Date().toISOString(),
            isSelf: true, 
        })
    );

    const broadcastMessage = {
        type: "message",
        sender: sender,
        senderId: ws.sessionId, 
        content: messageContent,
        frequency: frequency, 
        messageId,
        timestamp: new Date().toISOString(),
    };
    const broadcastStr = JSON.stringify(broadcastMessage);
    connectionManager.broadcast(frequency, broadcastStr, ws);

    wavelength.clients.forEach((client) => {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
            client.send(broadcastStr);
        }
    });

    if (!isHost && wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
        wavelength.host.send(broadcastStr);
    }
}

/**
 * File upload support
 * @param {WebSocket} ws - WebSocket connection (ws.frequency is string "XXX.X")
 * @param {Object} data - File data
 */
function handleSendFile(ws, data) {
  const frequency = ws.frequency;
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
 * @param {WebSocket} ws - WebSocket connection (ws.frequency is string "XXX.X")
 * @param {Object} data - Request data (may be empty)
 */
async function handleLeaveWavelength(ws, data) {
    const frequency = ws.frequency; 
    const sessionId = ws.sessionId || 'unknown';

    if (!frequency) {
        console.log(`Handler: Client ${sessionId} sent leave_wavelength but was not on a frequency.`);
        return;
    }

    console.log(`Handler: Client ${sessionId} attempting to leave frequency ${frequency}`);

    connectionManager.handleDisconnect(ws);
    ws.frequency = null;
    ws.isHost = false;
    ws.send(JSON.stringify({ type: "leave_result", success: true, frequency: frequency }));

    console.log(`Handler: Initiated disconnect logic for ${sessionId} leaving ${frequency}.`);
}


/**
 * Handler for closing wavelength (only host can do this)
 * @param {WebSocket} ws - WebSocket connection (ws.frequency is string "XXX.X")
 * @param {Object} data - Request data (may be empty)
 */
async function handleCloseWavelength(ws, data) {
    const frequency = ws.frequency; 
    const sessionId = ws.sessionId || 'unknown';

    if (!frequency) {
        console.error(`Handler: close_wavelength attempt from ${sessionId} not connected to a frequency.`);
        ws.send(JSON.stringify({ type: "error", error: "Cannot close: Not connected to a frequency" }));
        return;
    }

    const wavelength = connectionManager.activeWavelengths.get(frequency);

    if (!wavelength) {
        console.warn(`Handler: Wavelength ${frequency} not found for close_wavelength request from ${sessionId}. Might be already closed.`);
        ws.send(JSON.stringify({ type: "error", error: "Wavelength not found or already closed" }));
        ws.frequency = null;
        ws.isHost = false;
        return;
    }

    if (!ws.isHost || ws !== wavelength.host) {
        console.error(`Handler: Unauthorized close_wavelength attempt on ${frequency} by ${sessionId}. Expected host: ${wavelength.sessionId}`);
        ws.send(JSON.stringify({ type: "error", error: "Only the host can close the wavelength" }));
        return;
    }

    console.log(`Handler: Host ${sessionId} requested to close wavelength ${frequency}`);

    try {
        const closed = await wavelengthService.closeWavelength(frequency, "Host closed the wavelength");
        if (closed) {
            console.log(`Handler: Wavelength ${frequency} closed successfully by service.`);
            ws.send(
                JSON.stringify({
                    type: "close_result",
                    success: true,
                    frequency: frequency, 
                })
            );
        } else {
            console.error(`Handler: Service failed to close wavelength ${frequency}.`);
            ws.send(JSON.stringify({ type: "close_result", success: false, error: "Failed to close wavelength", frequency: frequency }));
        }
    } catch (err) {
        console.error(`Handler: Error calling service to close wavelength ${frequency}:`, err);
        ws.send(JSON.stringify({ type: "close_result", success: false, error: "Server error during close", frequency: frequency }));
    }
}

/**
 * Handles PTT request from a client.
 * @param {WebSocket} ws - WebSocket connection.
 * @param {object} data - Request data { frequency }.
 */
function handleRequestPtt(ws, data) {
    const frequency = ws.frequency; 
    const requestingSessionId = ws.sessionId;

    if (!frequency || data.frequency !== frequency) {
        console.warn(`Handler: PTT request frequency mismatch or missing. WS Freq: ${frequency}, Data Freq: ${data.frequency}, Session: ${requestingSessionId}`);
        ws.send(JSON.stringify({ type: "ptt_denied", frequency: data.frequency || frequency, reason: "Invalid request data." }));
        return;
    }

    const wavelength = connectionManager.activeWavelengths.get(frequency);
    if (!wavelength) {
        console.warn(`Handler: PTT request for non-active wavelength ${frequency}. Session: ${requestingSessionId}`);
        ws.send(JSON.stringify({ type: "ptt_denied", frequency: frequency, reason: "Wavelength not active." }));
        return;
    }

    if (wavelength.pttTransmitter === null) {
        wavelength.pttTransmitter = ws;
        console.log(`Handler: PTT Granted for ${requestingSessionId} on ${frequency}`);
        ws.send(JSON.stringify({ type: "ptt_granted", frequency: frequency }));
        const startReceivingMsg = JSON.stringify({
            type: "ptt_start_receiving",
            frequency: frequency,
            senderId: requestingSessionId
        });
        connectionManager.broadcast(frequency, startReceivingMsg, ws); 
    } else if (wavelength.pttTransmitter === ws) {
        console.log(`Handler: PTT Granted (already transmitting) for ${requestingSessionId} on ${frequency}`);
        ws.send(JSON.stringify({ type: "ptt_granted", frequency: frequency }));
    }
    else {
        const currentTransmitterId = wavelength.pttTransmitter.sessionId || 'Unknown';
        console.log(`Handler: PTT Denied for ${requestingSessionId} on ${frequency} (Busy - ${currentTransmitterId})`);
        ws.send(JSON.stringify({ type: "ptt_denied", frequency: frequency, reason: `Transmission slot busy (User ${currentTransmitterId.substring(0,8)})` }));
    }
}

/**
 * Handles PTT release from a client.
 * @param {WebSocket} ws - WebSocket connection.
 * @param {object} data - Request data { frequency }.
 */
function handleReleasePtt(ws, data) {
    const frequency = ws.frequency;
    const releasingSessionId = ws.sessionId;

    if (!frequency || data.frequency !== frequency) {
        console.warn(`Handler: PTT release frequency mismatch or missing. WS Freq: ${frequency}, Data Freq: ${data.frequency}, Session: ${releasingSessionId}`);
        return; 
    }

    const wavelength = connectionManager.activeWavelengths.get(frequency);
    if (!wavelength) {
        console.warn(`Handler: PTT release for non-active wavelength ${frequency}. Session: ${releasingSessionId}`);
        return;
    }

    if (wavelength.pttTransmitter === ws) {
        wavelength.pttTransmitter = null;
        console.log(`Handler: PTT Released by ${releasingSessionId} on ${frequency}`);
        const stopReceivingMsg = JSON.stringify({
            type: "ptt_stop_receiving",
            frequency: frequency
        });
        connectionManager.broadcast(frequency, stopReceivingMsg, ws); 
    } else {
        console.log(`Handler: PTT Release ignored for ${releasingSessionId} on ${frequency} (was not the transmitter). Current: ${wavelength.pttTransmitter?.sessionId}`);
    }
}

/**
 * Handles incoming binary audio data.
 * @param {WebSocket} ws - WebSocket connection.
 * @param {Buffer} message - Binary audio data.
 */
function handleAudioData(ws, message) {
    const frequency = ws.frequency;
    const senderSessionId = ws.sessionId;

    console.log(`[Server] handleAudioData: Received ${message.length} binary bytes from ${senderSessionId} on ${frequency}`);

    if (!frequency) {
        console.warn(`[Server] handleAudioData: Sender ${senderSessionId} has no frequency.`);
        return;
    }

    const wavelength = connectionManager.activeWavelengths.get(frequency);
    if (!wavelength) {
        console.warn(`[Server] handleAudioData: Wavelength ${frequency} not active for sender ${senderSessionId}.`);
        return;
    }

    if (wavelength.pttTransmitter === ws) {
        console.log(`[Server] handleAudioData: Sender ${senderSessionId} IS the PTT transmitter. Broadcasting...`);
        connectionManager.broadcast(frequency, message, ws); 
    } else {
        const currentTransmitterId = wavelength.pttTransmitter ? wavelength.pttTransmitter.sessionId : 'none';
        console.warn(`[Server] handleAudioData: Sender ${senderSessionId} is NOT the PTT transmitter (Current: ${currentTransmitterId}). Ignoring data.`);
    }
}


/**
 * Main handler for WebSocket messages
 * @param {WebSocket} ws - WebSocket connection
 * @param {Buffer|string} message - Received message
 */
async function handleMessage(ws, messageData) {
    let data;
    let isLikelyJson = false;
    let messageString = '';

    if (Buffer.isBuffer(messageData)) {
        try {
            messageString = messageData.toString('utf8');
        } catch (e) {
            console.error("Handler: Error converting buffer to string:", e);
            messageString = null;
        }
    } else if (typeof messageData === 'string') {
        messageString = messageData;
    } else {
        console.error(`Handler: Received unexpected message type: ${typeof messageData}`);
        messageString = null;
    }

    if (messageString !== null) {
        try {
            data = JSON.parse(messageString);
            isLikelyJson = true;
        } catch (e) {
            isLikelyJson = false;
        }
    }

    if (isLikelyJson && data && typeof data.type === 'string') {
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
            case "request_ptt":
                handleRequestPtt(ws, data);
                break;
            case "release_ptt":
                handleReleasePtt(ws, data);
                break;
            default:
                console.log(`Handler: Received unknown JSON message type: ${data.type}`);
                ws.send(JSON.stringify({ type: "error", error: `Unknown message type: ${data.type}` }));
        }
    } else if (Buffer.isBuffer(messageData)) {
        handleAudioData(ws, messageData);
    } else {
        console.warn("Handler: Received non-JSON, non-binary message:", messageString ? messageString.substring(0, 100) + '...' : '<Could not convert to string>');
        ws.send(JSON.stringify({ type: "error", error: "Received unparseable message format" }));
    }
}

module.exports = {
    handleMessage,
};
