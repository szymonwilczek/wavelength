// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\websocket\connectionManager.js
const WebSocket = require("ws");
const WavelengthModel = require("../models/wavelength"); // Uses string frequencies
const crypto = require("crypto");
const { normalizeFrequency } = require("../utils/helpers"); // Returns string "XXX.X"
const frequencyTracker = require("../services/frequencyGapTracker"); // Import tracker

/**
 * Class that manages WebSocket connections and application state
 */
class ConnectionManager {
  constructor() {
    // Map storing active frequencies (string keys "XXX.X") and their info
    this.activeWavelengths = new Map();

    // Stores the last recorded frequency as a string "XXX.X"
    this.lastRegisteredFrequency = "130.0";

    this.pingInterval = 30000; // Ping interval in milliseconds
    this.heartbeatInterval = null; // Interval for heartbeat
  }

  /**
   * Initiates pinging of customers to maintain connections
   * @param {WebSocket.Server} wss - WebSocket server instance
   */
  startHeartbeat(wss) {
    console.log(`Starting heartbeat with interval: ${this.pingInterval}ms`);
    clearInterval(this.heartbeatInterval); // Clear existing interval if any
    this.heartbeatInterval = setInterval(() => {
      wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
          console.log(`Heartbeat: Client unresponsive (freq: ${ws.frequency || 'none'}), terminating.`);
          this.handleDisconnect(ws); // Handle disconnect logic
          return ws.terminate();
        }
        ws.isAlive = false;
        ws.ping(() => {}); // Add empty callback to prevent error on some Node versions
      });
    }, this.pingInterval);
  }

  /**
   * Stops the heartbeat interval
   */
  stopHeartbeat() {
    console.log("Stopping heartbeat");
    clearInterval(this.heartbeatInterval);
    this.heartbeatInterval = null;
  }

  /**
   * Handles client disconnection
   * @param {WebSocket} ws - WebSocket instance of the client (ws.frequency is string "XXX.X")
   */
  handleDisconnect(ws) {
    const frequency = ws.frequency; // Should be normalized string "XXX.X" or null
    const sessionId = ws.sessionId || 'unknown';

    if (!frequency) {
      console.log(`Client ${sessionId} disconnected (was not on a frequency).`);
      return;
    }

    console.log(`Client ${sessionId} disconnected from frequency ${frequency}`);

    const wavelength = this.activeWavelengths.get(frequency); // Use string key
    if (!wavelength) {
      console.log(`Wavelength ${frequency} not found in memory during disconnect of ${sessionId}.`);
      // Maybe it was already closed, or state is inconsistent.
      // Ensure frequency is marked as free in tracker if this client was the host
      // This is tricky, as we don't know for sure if ws *was* the host if wavelength is gone.
      // Rely on closeWavelength logic to handle tracker updates.
      return;
    }

    // --- NOWA LOGIKA PTT PRZY ROZŁĄCZENIU ---
    // Jeśli rozłączający się klient był nadawcą PTT, zwolnij slot
    if (wavelength.pttTransmitter === ws) {
      console.log(`ConnectionManager: PTT Transmitter ${sessionId} disconnected from ${frequency}. Releasing slot.`);
      wavelength.pttTransmitter = null;
      // Powiadom pozostałych klientów, że transmisja się zakończyła
      const stopMessage = JSON.stringify({
        type: "ptt_stop_receiving",
        frequency: frequency
      });
      // Wyślij do hosta (jeśli nie jest rozłączającym się)
      if (wavelength.host && wavelength.host !== ws && wavelength.host.readyState === WebSocket.OPEN) {
        wavelength.host.send(stopMessage);
      }
      // Wyślij do pozostałych klientów
      wavelength.clients.forEach(client => {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
          client.send(stopMessage);
        }
      });
    }
    // --- KONIEC NOWEJ LOGIKI PTT ---

    // If the client is the host, trigger wavelength closure
    if (ws.isHost && ws === wavelength.host) {
      console.log(`Host ${sessionId} disconnected from wavelength ${frequency}. Closing wavelength.`);
      // Use the closeWavelength function from handlers (or service) to ensure consistency
      // Avoid circular dependency: handlers -> connectionManager -> handlers
      // Let's call the service method directly.
      const wavelengthService = require('../services/wavelengthService'); // Local require to avoid cycle
      wavelengthService.closeWavelength(frequency, "Host disconnected")
          .catch(err => console.error(`Error closing wavelength ${frequency} after host disconnect:`, err));
      // Note: closeWavelength will handle memory, DB, and tracker cleanup.
    } else if (!ws.isHost) {
      // If it is a client (not a host), just remove it from the list
      const deleted = wavelength.clients.delete(ws);
      if (deleted) {
        console.log(`Removed client ${sessionId} from wavelength ${frequency}. Clients remaining: ${wavelength.clients.size}`);
        // Notify the host that the client has disconnected
        if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
          wavelength.host.send(
              JSON.stringify({
                type: "client_disconnected",
                frequency: frequency, // Send string freq
                sessionId: sessionId,
              })
          );
        }
      } else {
        console.log(`Client ${sessionId} not found in client list for ${frequency} during disconnect.`);
      }
    } else {
      // This case (ws.isHost but ws !== wavelength.host) should ideally not happen
      console.warn(`Disconnecting client ${sessionId} marked as host for ${frequency}, but doesn't match stored host!`);
      // Treat as regular client disconnect for safety?
      wavelength.clients.delete(ws);
    }
  }

  /**
   * Registers a new wavelength in memory
   * @param {WebSocket} ws - WebSocket instance of the host
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @param {string} name - Name of the wavelength
   * @param {boolean} isPasswordProtected - Whether the wavelength is password protected
   * @param {string|null} password - Raw password for the wavelength (if applicable)
   * @param {string} sessionId - Host's session ID
   * @returns {Object} - Information about the registered wavelength { frequency: string, sessionId: string }
   */
  registerWavelength(ws, frequency, name, isPasswordProtected, password, sessionId) {
    // frequency should already be normalized by caller (service/handler)
    ws.frequency = frequency;
    ws.isHost = true;
    ws.sessionId = sessionId;
    const hashedPassword = (isPasswordProtected && password) ?
        crypto.createHash('sha256').update(password).digest('hex') :
        null;

    this.activeWavelengths.set(frequency, { // Use string key
      host: ws,
      clients: new Set(),
      name,
      isPasswordProtected,
      password: hashedPassword, // Store hash
      sessionId, // Store host session ID
      processedMessageIds: new Set(),
        pttTransmitter: null, // PTT transmitter (if any)
    });

    // Update last recorded frequency (compare numerically, store string)
    try {
      if (parseFloat(frequency) > parseFloat(this.lastRegisteredFrequency)) {
        this.lastRegisteredFrequency = frequency;
      }
    } catch (e) {
      console.error(`Error comparing frequencies for lastRegisteredFrequency update: ${frequency} vs ${this.lastRegisteredFrequency}`, e);
    }


    console.log(`ConnectionManager: Registered wavelength ${frequency} in memory for host ${sessionId}.`);
    return { frequency, sessionId }; // Return string freq
  }

  /**
   * Adds client to existing wavelength in memory
   * @param {WebSocket} ws - WebSocket instance of the client
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @param {string} sessionId - Client's session ID
   * @returns {Object|null} - Info { frequency: string, name: string, sessionId: string } or null if wavelength not found
   */
  addClientToWavelength(ws, frequency, sessionId) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) {
      console.error(`ConnectionManager: Attempted to add client ${sessionId} to non-existent wavelength ${frequency}`);
      return null; // Indicate failure
    }

    ws.frequency = frequency;
    ws.isHost = false;
    ws.sessionId = sessionId;

    wavelength.clients.add(ws);
    console.log(`ConnectionManager: Added client ${sessionId} to wavelength ${frequency}. Total clients: ${wavelength.clients.size}`);

    // Notify host and other clients about user joining (optional)
    const joinMessage = JSON.stringify({
      type: "user_joined",
      frequency: frequency,
      userId: sessionId,
      timestamp: new Date().toISOString()
    });
    if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
      wavelength.host.send(joinMessage);
    }
    wavelength.clients.forEach(client => {
      // Don't send to the joining client itself
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(joinMessage);
      }
    });


    return { frequency, name: wavelength.name, sessionId }; // Return some info
  }

  /**
   * Deletes wavelength from memory ONLY. DB/Tracker handled by service/handler.
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @returns {boolean} - Whether the wavelength was found and removed from memory
   */
  removeWavelengthFromMemory(frequency) {
    // frequency should already be normalized by caller
    const deleted = this.activeWavelengths.delete(frequency); // Use string key
    if (deleted) {
      console.log(`ConnectionManager: Removed wavelength ${frequency} from memory.`);
    } else {
      console.log(`ConnectionManager: Wavelength ${frequency} not found in memory for removal.`);
    }
    return deleted;
  }


  /**
   * Retrieves wavelength info (checks memory first, then database)
   * @param {string} frequency - Frequency string ("XXX.X")
   * @returns {Promise<Object|null>} - Wavelength info object (frequency as string, includes isOnline flag) or null
   */
  async getWavelength(frequency) {
    let normalizedFrequency;
    try {
      normalizedFrequency = normalizeFrequency(frequency); // Ensure string format "XXX.X"
    } catch (e) {
      console.error(`getWavelength: Invalid frequency format: ${frequency}`);
      return null;
    }

    // Memory check
    if (this.activeWavelengths.has(normalizedFrequency)) { // Use string key
      const memWl = this.activeWavelengths.get(normalizedFrequency);
      console.log(`getWavelength: Found ${normalizedFrequency} in memory.`);
      return {
        frequency: normalizedFrequency,
        name: memWl.name,
        isPasswordProtected: memWl.isPasswordProtected,
        hostSocketId: memWl.sessionId, // Host's session ID
        passwordHash: memWl.password, // Return hash
        createdAt: null, // Not typically stored in memory map, get from DB if needed
        isOnline: true // Indicate it's active in memory
      };
    }

    // Database check
    console.log(`getWavelength: ${normalizedFrequency} not in memory, checking DB.`);
    try {
      const dbWavelength = await WavelengthModel.findByFrequency(normalizedFrequency); // Use string freq
      if (dbWavelength) {
        console.log(`getWavelength: Found ${normalizedFrequency} in DB.`);
        // Ensure frequency is string, add isOnline flag
        // Map DB fields to consistent names
        return {
          frequency: String(dbWavelength.frequency),
          name: dbWavelength.name,
          isPasswordProtected: dbWavelength.isPasswordProtected, // Already mapped in model? Check model. Yes.
          hostSocketId: dbWavelength.hostSocketId,
          passwordHash: dbWavelength.passwordHash,
          createdAt: dbWavelength.createdAt,
          isOnline: false // Indicate from DB, host not connected
        };
      }
      console.log(`getWavelength: ${normalizedFrequency} not found in DB.`);
      return null;
    } catch (error) {
      console.error(`getWavelength: Error finding wavelength ${normalizedFrequency} in database:`, error);
      return null;
    }
  }

  /**
   * Checks if the message has already been processed for a given frequency string
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @param {string} messageId - ID of the message
   * @returns {boolean} - Whether the message has been processed
   */
  isMessageProcessed(frequency, messageId) {
    // frequency should already be normalized by caller
    const wavelength = this.activeWavelengths.get(frequency); // Use string key
    if (!wavelength) return false; // Wavelength gone, message not processed by it

    return wavelength.processedMessageIds.has(messageId);
  }

  /**
   * Marks the message as processed for a given frequency string
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @param {string} messageId - ID of the message
   */
  markMessageProcessed(frequency, messageId) {
    // frequency should already be normalized by caller
    const wavelength = this.activeWavelengths.get(frequency); // Use string key
    if (!wavelength) return;

    wavelength.processedMessageIds.add(messageId);

    // Limit size of the processed messages set
    const maxSize = 1000;
    const trimSize = 200;
    if (wavelength.processedMessageIds.size > maxSize) {
      console.log(`Trimming processedMessageIds for frequency ${frequency} (size: ${wavelength.processedMessageIds.size})`);
      const iterator = wavelength.processedMessageIds.values();
      for (let i = 0; i < trimSize; i++) {
        const next = iterator.next();
        if (!next.done) {
          wavelength.processedMessageIds.delete(next.value);
        } else {
          break; // Stop if set becomes smaller than expected
        }
      }
      console.log(`Trimmed processedMessageIds for frequency ${frequency} (new size: ${wavelength.processedMessageIds.size})`);
    }
  }

  /**
   * Broadcasts a message to all connected clients on a specific frequency,
   * optionally excluding the sender.
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @param {string|Buffer} message - The message to broadcast (JSON string or binary Buffer)
   * @param {WebSocket} [senderWs=null] - The WebSocket instance of the sender (to exclude)
   */
  broadcast(frequency, message, senderWs = null) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) {
      console.warn(`[Server] Broadcast: Wavelength ${frequency} not active.`);
      return;
    }

    const isBinary = Buffer.isBuffer(message);
    const senderId = senderWs ? senderWs.sessionId : 'N/A';
    // --- DODANE LOGOWANIE ---
    console.log(`[Server] Broadcast: Broadcasting ${isBinary ? 'binary (' + message.length + ' bytes)' : 'text'} on ${frequency} from ${senderId}. Excluding sender: ${!!senderWs}`);
    console.log(`[Server] Broadcast: Wavelength ${frequency} has host: ${wavelength.host ? wavelength.host.sessionId : 'none'}, clients: ${wavelength.clients.size}`);
    // --- KONIEC LOGOWANIA ---


    // Send to host (if exists, is not the sender, and is open)
    if (wavelength.host && wavelength.host !== senderWs && wavelength.host.readyState === WebSocket.OPEN) {
      try {
        // --- DODANE LOGOWANIE ---
        console.log(`[Server] Broadcast: Sending to host ${wavelength.host.sessionId}`);
        // --- KONIEC LOGOWANIA ---
        wavelength.host.send(message);
      } catch (e) {
        console.error(`[Server] Broadcast Error sending to host on ${frequency}:`, e);
      }
    } else if (wavelength.host === senderWs) {
      console.log(`[Server] Broadcast: Host ${senderId} is the sender, not broadcasting back.`);
    }
    // Add more detailed logs for why host might not receive if needed


    // Send to other clients (if they exist, are not the sender, and are open)
    wavelength.clients.forEach(client => {
      if (client !== senderWs && client.readyState === WebSocket.OPEN) {
        try {
          // --- DODANE LOGOWANIE ---
          console.log(`[Server] Broadcast: Sending to client ${client.sessionId}`);
          // --- KONIEC LOGOWANIA ---
          client.send(message);
        } catch (e) {
          console.error(`[Server] Broadcast Error sending to client ${client.sessionId} on ${frequency}:`, e);
        }
      } else if (client === senderWs) {
        // console.log(`[Server] Broadcast: Client ${client.sessionId} is the sender, not broadcasting back.`); // Can be noisy
      } else if (client.readyState !== WebSocket.OPEN) {
        console.log(`[Server] Broadcast: Client ${client.sessionId} not OPEN (state: ${client.readyState}). Not sending.`);
      } else {
        console.log(`[Server] Broadcast: Skipping client ${client.sessionId} (unknown reason, possibly sender check failed unexpectedly).`);
      }
    });

    if (wavelength.clients.size === 0 && wavelength.host !== senderWs) { // Check if there were any potential recipients other than sender
      console.log(`[Server] Broadcast: No other clients or host connected to ${frequency} to broadcast to.`);
    } else if (wavelength.clients.size === 0 && wavelength.host === senderWs) {
      console.log(`[Server] Broadcast: Host was sender, no other clients connected to ${frequency}.`);
    }
  }

}

// Export singleton instance
module.exports = new ConnectionManager();