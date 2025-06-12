const WebSocket = require("ws");
const WavelengthModel = require("../models/wavelength"); 
const crypto = require("crypto");
const { normalizeFrequency } = require("../utils/helpers");
const frequencyTracker = require("../services/frequencyGapTracker"); 

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
    clearInterval(this.heartbeatInterval); 
    this.heartbeatInterval = setInterval(() => {
      wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
          console.log(`Heartbeat: Client unresponsive (freq: ${ws.frequency || 'none'}), terminating.`);
          this.handleDisconnect(ws); 
          return ws.terminate();
        }
        ws.isAlive = false;
        ws.ping(() => {});
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
    const frequency = ws.frequency; 
    const sessionId = ws.sessionId || 'unknown';

    if (!frequency) {
      console.log(`Client ${sessionId} disconnected (was not on a frequency).`);
      return;
    }

    console.log(`Client ${sessionId} disconnected from frequency ${frequency}`);

    const wavelength = this.activeWavelengths.get(frequency); 
    if (!wavelength) {
      console.log(`Wavelength ${frequency} not found in memory during disconnect of ${sessionId}.`);
      return;
    }

    if (wavelength.pttTransmitter === ws) {
      console.log(`ConnectionManager: PTT Transmitter ${sessionId} disconnected from ${frequency}. Releasing slot.`);
      wavelength.pttTransmitter = null;
      const stopMessage = JSON.stringify({
        type: "ptt_stop_receiving",
        frequency: frequency
      });
      if (wavelength.host && wavelength.host !== ws && wavelength.host.readyState === WebSocket.OPEN) {
        wavelength.host.send(stopMessage);
      }
      wavelength.clients.forEach(client => {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
          client.send(stopMessage);
        }
      });
    }

    if (ws.isHost && ws === wavelength.host) {
      console.log(`Host ${sessionId} disconnected from wavelength ${frequency}. Closing wavelength.`);
      const wavelengthService = require('../services/wavelengthService'); 
      wavelengthService.closeWavelength(frequency, "Host disconnected")
          .catch(err => console.error(`Error closing wavelength ${frequency} after host disconnect:`, err));
    } else if (!ws.isHost) {
      const deleted = wavelength.clients.delete(ws);
      if (deleted) {
        console.log(`Removed client ${sessionId} from wavelength ${frequency}. Clients remaining: ${wavelength.clients.size}`);
        if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
          wavelength.host.send(
              JSON.stringify({
                type: "client_disconnected",
                frequency: frequency, 
                sessionId: sessionId,
              })
          );
        }
      } else {
        console.log(`Client ${sessionId} not found in client list for ${frequency} during disconnect.`);
      }
    } else {
      console.warn(`Disconnecting client ${sessionId} marked as host for ${frequency}, but doesn't match stored host!`);
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
    ws.frequency = frequency;
    ws.isHost = true;
    ws.sessionId = sessionId;
    const hashedPassword = (isPasswordProtected && password) ?
        crypto.createHash('sha256').update(password).digest('hex') :
        null;

    this.activeWavelengths.set(frequency, { 
      host: ws,
      clients: new Set(),
      name,
      isPasswordProtected,
      password: hashedPassword, 
      sessionId,
      processedMessageIds: new Set(),
        pttTransmitter: null, 
    });

    try {
      if (parseFloat(frequency) > parseFloat(this.lastRegisteredFrequency)) {
        this.lastRegisteredFrequency = frequency;
      }
    } catch (e) {
      console.error(`Error comparing frequencies for lastRegisteredFrequency update: ${frequency} vs ${this.lastRegisteredFrequency}`, e);
    }


    console.log(`ConnectionManager: Registered wavelength ${frequency} in memory for host ${sessionId}.`);
    return { frequency, sessionId }; 
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
      return null; 
    }

    ws.frequency = frequency;
    ws.isHost = false;
    ws.sessionId = sessionId;

    wavelength.clients.add(ws);
    console.log(`ConnectionManager: Added client ${sessionId} to wavelength ${frequency}. Total clients: ${wavelength.clients.size}`);

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
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        client.send(joinMessage);
      }
    });


    return { frequency, name: wavelength.name, sessionId }; 
  }

  /**
   * Deletes wavelength from memory ONLY. DB/Tracker handled by service/handler.
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @returns {boolean} - Whether the wavelength was found and removed from memory
   */
  removeWavelengthFromMemory(frequency) {
    const deleted = this.activeWavelengths.delete(frequency); 
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
      normalizedFrequency = normalizeFrequency(frequency); 
    } catch (e) {
      console.error(`getWavelength: Invalid frequency format: ${frequency}`);
      return null;
    }

    if (this.activeWavelengths.has(normalizedFrequency)) { 
      const memWl = this.activeWavelengths.get(normalizedFrequency);
      console.log(`getWavelength: Found ${normalizedFrequency} in memory.`);
      return {
        frequency: normalizedFrequency,
        name: memWl.name,
        isPasswordProtected: memWl.isPasswordProtected,
        hostSocketId: memWl.sessionId, 
        passwordHash: memWl.password, 
        createdAt: null, 
        isOnline: true 
      };
    }

    console.log(`getWavelength: ${normalizedFrequency} not in memory, checking DB.`);
    try {
      const dbWavelength = await WavelengthModel.findByFrequency(normalizedFrequency); 
      if (dbWavelength) {
        console.log(`getWavelength: Found ${normalizedFrequency} in DB.`);
        return {
          frequency: String(dbWavelength.frequency),
          name: dbWavelength.name,
          isPasswordProtected: dbWavelength.isPasswordProtected, 
          hostSocketId: dbWavelength.hostSocketId,
          passwordHash: dbWavelength.passwordHash,
          createdAt: dbWavelength.createdAt,
          isOnline: false 
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
    const wavelength = this.activeWavelengths.get(frequency); 
    if (!wavelength) return false;

    return wavelength.processedMessageIds.has(messageId);
  }

  /**
   * Marks the message as processed for a given frequency string
   * @param {string} frequency - Normalized frequency string ("XXX.X")
   * @param {string} messageId - ID of the message
   */
  markMessageProcessed(frequency, messageId) {
    const wavelength = this.activeWavelengths.get(frequency); 
    if (!wavelength) return;

    wavelength.processedMessageIds.add(messageId);

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
          break; // stop if set becomes smaller than expected
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
    console.log(`[Server] Broadcast: Broadcasting ${isBinary ? 'binary (' + message.length + ' bytes)' : 'text'} on ${frequency} from ${senderId}. Excluding sender: ${!!senderWs}`);
    console.log(`[Server] Broadcast: Wavelength ${frequency} has host: ${wavelength.host ? wavelength.host.sessionId : 'none'}, clients: ${wavelength.clients.size}`);

    if (wavelength.host && wavelength.host !== senderWs && wavelength.host.readyState === WebSocket.OPEN) {
      try {
        console.log(`[Server] Broadcast: Sending to host ${wavelength.host.sessionId}`);
        wavelength.host.send(message);
      } catch (e) {
        console.error(`[Server] Broadcast Error sending to host on ${frequency}:`, e);
      }
    } else if (wavelength.host === senderWs) {
      console.log(`[Server] Broadcast: Host ${senderId} is the sender, not broadcasting back.`);
    }

    wavelength.clients.forEach(client => {
      if (client !== senderWs && client.readyState === WebSocket.OPEN) {
        try {
          console.log(`[Server] Broadcast: Sending to client ${client.sessionId}`);
          client.send(message);
        } catch (e) {
          console.error(`[Server] Broadcast Error sending to client ${client.sessionId} on ${frequency}:`, e);
        }
      } else if (client === senderWs) {
      } else if (client.readyState !== WebSocket.OPEN) {
        console.log(`[Server] Broadcast: Client ${client.sessionId} not OPEN (state: ${client.readyState}). Not sending.`);
      } else {
        console.log(`[Server] Broadcast: Skipping client ${client.sessionId} (unknown reason, possibly sender check failed unexpectedly).`);
      }
    });

    if (wavelength.clients.size === 0 && wavelength.host !== senderWs) { 
      console.log(`[Server] Broadcast: No other clients or host connected to ${frequency} to broadcast to.`);
    } else if (wavelength.clients.size === 0 && wavelength.host === senderWs) {
      console.log(`[Server] Broadcast: Host was sender, no other clients connected to ${frequency}.`);
    }
  }

}

module.exports = new ConnectionManager();
