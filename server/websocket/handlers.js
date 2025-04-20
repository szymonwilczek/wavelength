// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\websocket\handlers.js
const WebSocket = require("ws");
const crypto = require("crypto");
const WavelengthModel = require("../models/wavelength"); // Uses string frequencies
const connectionManager = require("./connectionManager"); // Uses string frequencies
const {
    normalizeFrequency, // Returns string "XXX.X"
    generateSessionId,
    generateMessageId,
    isValidFrequency // Checks if input can be normalized
} = require("../utils/helpers");
const wavelengthService = require("../services/wavelengthService"); // Uses string frequencies
// Removed frequencyTracker import as service layer handles it

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
    // Note: Service layer will normalize again, but validation here is good.
    const frequency = normalizeFrequency(rawFrequency); // For logging

    const name = data.name || `Wavelength-${frequency}`;
    const isPasswordProtected = data.isPasswordProtected || false;
    const password = data.password || null; // Use null if no password

    try {
        console.log(`Handler: Attempting to register frequency ${frequency}...`);

        // Use WavelengthService for registration logic
        const result = await wavelengthService.registerWavelength(
            ws,
            rawFrequency, // Pass raw (but validated) frequency to service
            name,
            isPasswordProtected,
            password
        );

        // Send result back to client
        ws.send(JSON.stringify({
            type: "register_result",
            ...result // Contains success, frequency (string), sessionId, error
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
    const frequency = normalizeFrequency(rawFrequency); // Normalized string for internal use

    const password = data.password || ""; // Keep as empty string if not provided

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
            const hashedStoredPassword = wavelengthInfo.password; // Get hash from memory store

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
                    frequency: joinResult.frequency, // Send string freq "XXX.X"
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
                        frequency: frequency, // Send string freq "XXX.X"
                    })
                );
            }
            console.log(`Handler: Client ${sessionId} joined wavelength ${frequency}`);
        } else {
            // This case indicates an internal issue if wavelengthInfo was found earlier
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
    const frequency = ws.frequency; // Should be normalized string "XXX.X" from join/register
    const messageContent = data.content || data.message; // Accept 'content' or 'message'
    const messageId = data.messageId || generateMessageId("msg");

    if (!frequency) {
        console.error(`Handler: send_message attempt from client ${ws.sessionId || 'unknown'} not connected to a frequency.`);
        ws.send(JSON.stringify({ type: "error", error: "Cannot send message: Not connected to a frequency" }));
        return;
    }
    if (!messageContent) {
        console.warn(`Handler: send_message received empty message content on ${frequency} from ${ws.sessionId || 'unknown'}.`);
        // Optionally send an error or just ignore
        // ws.send(JSON.stringify({ type: "error", error: "Cannot send empty message" }));
        return;
    }


    const wavelength = connectionManager.activeWavelengths.get(frequency); // Use string key

    if (!wavelength) {
        console.error(`Handler: Wavelength ${frequency} not found for send_message from ${ws.sessionId || 'unknown'}.`);
        ws.send(JSON.stringify({ type: "error", error: "Wavelength does not exist or host is offline" }));
        return;
    }

    // Check for duplicate messageId
    if (connectionManager.isMessageProcessed(frequency, messageId)) {
        console.log(`Handler: Ignoring duplicate message ${messageId} on ${frequency}`);
        return; // Silently ignore duplicates
    }
    connectionManager.markMessageProcessed(frequency, messageId);

    const isHost = ws.isHost;
    const sender = isHost ? "Host" : (ws.sessionId ? ws.sessionId.substring(0, 8) : "Client"); // Use prefix of sessionId for clients

    console.log(`Handler: Message on ${frequency} from ${sender} (ID: ${messageId}): ${messageContent}`);

    // Send confirmation back to sender (optional, useful for client UI)
    ws.send(
        JSON.stringify({
            type: "message",
            sender: "You", // Indicate it's the sender's own message
            content: messageContent,
            frequency: frequency, // Send string freq "XXX.X"
            messageId,
            timestamp: new Date().toISOString(),
            isSelf: true, // Flag for sender's client
        })
    );

    // Prepare broadcast message for others
    const broadcastMessage = {
        type: "message",
        sender: sender, // "Host" or client ID prefix
        senderId: ws.sessionId, // Include full session ID
        content: messageContent,
        frequency: frequency, // Send string freq "XXX.X"
        messageId,
        timestamp: new Date().toISOString(),
    };
    const broadcastStr = JSON.stringify(broadcastMessage);

    // Send to other clients on the same frequency
    wavelength.clients.forEach((client) => {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
            client.send(broadcastStr);
        }
    });

    // Send to host if the sender is a client
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
    const frequency = ws.frequency; // Should be normalized string "XXX.X"
    const messageId = data.messageId || generateMessageId("file");
    const attachmentType = data.attachmentType; // e.g., 'image', 'audio', 'video', 'file'
    const attachmentMimeType = data.attachmentMimeType; // e.g., 'image/png'
    const attachmentName = data.attachmentName; // e.g., 'photo.png'
    const attachmentData = data.attachmentData; // Base64 encoded string
    const senderId = ws.sessionId; // Use sender's session ID
    const timestamp = data.timestamp || Date.now(); // Use provided or current time

    if (!frequency) {
        console.error(`Handler: send_file attempt from client ${senderId || 'unknown'} not connected to a frequency.`);
        ws.send(JSON.stringify({ type: "error", error: "Cannot send file: Not connected to a frequency" }));
        return;
    }
    if (!attachmentData || !attachmentName || !attachmentType || !attachmentMimeType) {
        console.error(`Handler: send_file missing required fields on ${frequency} from ${senderId}.`);
        ws.send(JSON.stringify({ type: "error", error: "Missing required attachment data fields (type, mimeType, name, data)." }));
        return;
    }

    const wavelength = connectionManager.activeWavelengths.get(frequency); // Use string key

    if (!wavelength) {
        console.error(`Handler: Wavelength ${frequency} not found for send_file from ${senderId}.`);
        ws.send(JSON.stringify({ type: "error", error: "Wavelength does not exist or host is offline" }));
        return;
    }

    // Approximate size check based on base64 length (adjust limit as needed)
    const maxSizeMb = 20;
    const maxBase64Length = maxSizeMb * 1024 * 1024 * 4 / 3; // Base64 is ~4/3 larger
    if (attachmentData.length > maxBase64Length) {
        console.warn(`Handler: File size exceeds limit (${maxSizeMb}MB) for ${attachmentName} on ${frequency} from ${senderId}.`);
        ws.send(JSON.stringify({ type: "error", error: `File size exceeds the maximum limit (~${maxSizeMb}MB)` }));
        return;
    }

    // Check for duplicate messageId
    if (connectionManager.isMessageProcessed(frequency, messageId)) {
        console.log(`Handler: Ignoring duplicate file message ${messageId} on ${frequency}`);
        return; // Silently ignore
    }
    connectionManager.markMessageProcessed(frequency, messageId);

    const isHost = ws.isHost;
    const sender = isHost ? "Host" : (senderId ? senderId.substring(0, 8) : "Client");

    console.log(`Handler: File received on ${frequency} from ${sender} (ID: ${messageId}): ${attachmentName} (${attachmentType})`);

    // Send confirmation back to sender (optional)
    // Consider sending only metadata back, not the full data again
    ws.send(
        JSON.stringify({
            type: "message", // Still type message, indicates successful receipt/processing start
            hasAttachment: true,
            attachmentType: attachmentType,
            attachmentMimeType: attachmentMimeType,
            attachmentName: attachmentName,
            // attachmentData: attachmentData, // Avoid sending data back unless necessary
            frequency: frequency, // Send string freq "XXX.X"
            messageId: messageId,
            timestamp: new Date(timestamp).toISOString(),
            isSelf: true, // Flag for sender's client
            senderId: senderId,
        })
    );

    // Prepare broadcast message for others
    const broadcastMessage = {
        type: "message", // Clients expect 'message' type for chat display
        hasAttachment: true,
        attachmentType: attachmentType,
        attachmentMimeType: attachmentMimeType,
        attachmentName: attachmentName,
        attachmentData: attachmentData, // Include base64 data for others
        frequency: frequency, // Send string freq "XXX.X"
        messageId: messageId,
        timestamp: new Date(timestamp).toISOString(),
        sender: sender, // "Host" or client ID prefix
        senderId: senderId, // Full session ID of sender
    };
    const broadcastStr = JSON.stringify(broadcastMessage);

    // Send to other clients
    wavelength.clients.forEach((client) => {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
            try {
                client.send(broadcastStr);
            } catch (e) {
                console.error(`Handler: Error sending file message to client ${client.sessionId || 'unknown'}:`, e);
                // Consider removing client if send fails repeatedly?
            }
        }
    });

    // Send to host if sender is a client
    if (!isHost && wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
        try {
            wavelength.host.send(broadcastStr);
        } catch (e) {
            console.error("Handler: Error sending file message to host:", e);
        }
    }
}


/**
 * Handler for leaving wavelength
 * @param {WebSocket} ws - WebSocket connection (ws.frequency is string "XXX.X")
 * @param {Object} data - Request data (may be empty)
 */
async function handleLeaveWavelength(ws, data) {
    const frequency = ws.frequency; // Should be normalized string "XXX.X" or null
    const sessionId = ws.sessionId || 'unknown';

    if (!frequency) {
        console.log(`Handler: Client ${sessionId} sent leave_wavelength but was not on a frequency.`);
        // Optionally send confirmation even if not on frequency?
        // ws.send(JSON.stringify({ type: "leave_result", success: true, frequency: null }));
        return;
    }

    console.log(`Handler: Client ${sessionId} attempting to leave frequency ${frequency}`);

    // Let ConnectionManager handle the logic of removing client/closing wavelength if host
    // This avoids duplicating logic here and ensures cleanup happens correctly on disconnect too.
    // We just need to trigger the disconnect flow for this specific client.
    connectionManager.handleDisconnect(ws);

    // Resetting WebSocket properties immediately after triggering disconnect logic
    ws.frequency = null;
    ws.isHost = false;
    // ws.sessionId = null; // Keep sessionId for potential logging after disconnect

    // Send confirmation back to the client who requested to leave
    ws.send(JSON.stringify({ type: "leave_result", success: true, frequency: frequency }));

    console.log(`Handler: Initiated disconnect logic for ${sessionId} leaving ${frequency}.`);
}


/**
 * Handler for closing wavelength (only host can do this)
 * @param {WebSocket} ws - WebSocket connection (ws.frequency is string "XXX.X")
 * @param {Object} data - Request data (may be empty)
 */
async function handleCloseWavelength(ws, data) {
    const frequency = ws.frequency; // Should be normalized string "XXX.X" or null
    const sessionId = ws.sessionId || 'unknown';

    if (!frequency) {
        console.error(`Handler: close_wavelength attempt from ${sessionId} not connected to a frequency.`);
        ws.send(JSON.stringify({ type: "error", error: "Cannot close: Not connected to a frequency" }));
        return;
    }

    const wavelength = connectionManager.activeWavelengths.get(frequency); // Use string key

    if (!wavelength) {
        console.warn(`Handler: Wavelength ${frequency} not found for close_wavelength request from ${sessionId}. Might be already closed.`);
        ws.send(JSON.stringify({ type: "error", error: "Wavelength not found or already closed" }));
        // Reset client state just in case
        ws.frequency = null;
        ws.isHost = false;
        return;
    }

    // Verify the request comes from the actual host stored in memory
    if (!ws.isHost || ws !== wavelength.host) {
        console.error(`Handler: Unauthorized close_wavelength attempt on ${frequency} by ${sessionId}. Expected host: ${wavelength.sessionId}`);
        ws.send(JSON.stringify({ type: "error", error: "Only the host can close the wavelength" }));
        return;
    }

    console.log(`Handler: Host ${sessionId} requested to close wavelength ${frequency}`);

    // Use the service layer to handle the closing process (notifies clients, cleans DB/tracker/memory)
    try {
        const closed = await wavelengthService.closeWavelength(frequency, "Host closed the wavelength");
        if (closed) {
            console.log(`Handler: Wavelength ${frequency} closed successfully by service.`);
            // Send confirmation back to host
            ws.send(
                JSON.stringify({
                    type: "close_result",
                    success: true,
                    frequency: frequency, // Send string freq "XXX.X"
                })
            );
            // Host state (ws.frequency etc.) will be reset by the disconnect flow triggered by closeWavelength
        } else {
            console.error(`Handler: Service failed to close wavelength ${frequency}.`);
            ws.send(JSON.stringify({ type: "close_result", success: false, error: "Failed to close wavelength", frequency: frequency }));
        }
    } catch (err) {
        console.error(`Handler: Error calling service to close wavelength ${frequency}:`, err);
        ws.send(JSON.stringify({ type: "close_result", success: false, error: "Server error during close", frequency: frequency }));
    }
}


// Removed standalone closeWavelength function from here, use wavelengthService.closeWavelength


/**
 * Main handler for WebSocket messages
 * @param {WebSocket} ws - WebSocket connection
 * @param {Buffer|string} message - Received message
 */
async function handleMessage(ws, message) {
    let data;
    const messageString = message.toString(); // Convert buffer/string to string once

    try {
        // console.log("Received raw:", messageString); // Verbose: Log raw message
        data = JSON.parse(messageString);

        if (!data || typeof data !== 'object' || !data.type) {
            throw new Error("Invalid message format or missing type field");
        }

    } catch (e) {
        console.error("Error parsing message or invalid format:", e, `Raw: ${messageString.substring(0, 100)}...`); // Log truncated raw msg on error
        ws.send(JSON.stringify({ type: "error", error: "Invalid message format or missing type" }));
        return;
    }

    try {
        // Log processed message type and relevant context
        const context = ws.frequency ? `(Freq: ${ws.frequency}, SID: ${ws.sessionId || 'unknown'})` : `(SID: ${ws.sessionId || 'unknown'})`;
        console.log(`Handler: Processing type '${data.type}' ${context}`);

        switch (data.type) {
            case "register_wavelength":
                await handleRegisterWavelength(ws, data);
                break;
            case "join_wavelength":
                await handleJoinWavelength(ws, data);
                break;
            case "send_message":
                handleSendMessage(ws, data); // Internal check for ws.frequency
                break;
            case "send_file":
                handleSendFile(ws, data); // Internal check for ws.frequency
                break;
            case "leave_wavelength":
                await handleLeaveWavelength(ws, data); // Internal check for ws.frequency
                break;
            case "close_wavelength":
                await handleCloseWavelength(ws, data); // Internal check for ws.frequency
                break;
            // Add ping/pong handling? ws library handles basic pong response automatically.
            // case 'ping': ws.pong(); break;
            default:
                console.warn(`Handler: Received unknown message type: ${data.type} ${context}`);
                ws.send(JSON.stringify({ type: "error", error: `Unknown message type: ${data.type}` }));
        }
    } catch (e) {
        // Catch errors specific to handler logic execution
        const context = ws.frequency ? `(Freq: ${ws.frequency}, SID: ${ws.sessionId || 'unknown'})` : `(SID: ${ws.sessionId || 'unknown'})`;
        console.error(`Handler: Error processing message type ${data.type} ${context}:`, e);
        ws.send(JSON.stringify({ type: "error", error: `Server error processing message type ${data.type}` }));
    }
}

module.exports = {
    handleMessage,
    // No need to export individual handlers if handleMessage is the sole entry point
};