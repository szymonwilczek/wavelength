const WebSocket = require("ws");
const WavelengthModel = require("../models/wavelength");

/**
 * Class that manages WebSocket connections and application state
 */
class ConnectionManager {
  constructor() {
    // Map storing active frequencies and their info
    this.activeWavelengths = new Map();

    // Stores the last recorded frequency
    this.lastRegisteredFrequency = 130.0;

    // Ping interval in milliseconds
    this.pingInterval = 30000;

    // Interval for heartbeat
    this.heartbeatInterval = null;
  }

  /**
   * Initiates pinging of customers to maintain connections
   * @param {WebSocket.Server} wss - WebSocket server instance
   * @returns {void}
   */
  startHeartbeat(wss) {
    this.heartbeatInterval = setInterval(() => {
      wss.clients.forEach((ws) => {
        if (ws.isAlive === false) {
          this.handleDisconnect(ws);
          return ws.terminate();
        }

        ws.isAlive = false;
        ws.ping();
      });
    }, this.pingInterval);
  }

  /**
   * Stops the heartbeat interval
   * @returns {void}
   */
  stopHeartbeat() {
    clearInterval(this.heartbeatInterval);
  }

  /**
   * Handles client disconnection
   * @param {WebSocket} ws - WebSocket instance of the client
   * @returns {void}
   */
  handleDisconnect(ws) {
    if (!ws.frequency) return;

    console.log(`Client disconnected from frequency ${ws.frequency}`);

    const wavelength = this.activeWavelengths.get(ws.frequency);
    if (!wavelength) return;

    // If the client is the host, server will remove whole wavelength
    if (ws.isHost) {
      console.log(
        `Host disconnected from wavelength ${ws.frequency}, removing wavelength`
      );

      // Notify customers that wavelength has been removed
      wavelength.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(
            JSON.stringify({
              type: "wavelength_closed",
              message: "Host has disconnected",
            })
          );
        }
      });

      this.removeWavelength(ws.frequency);
    } else {
      // If it is a customer (not a host), just remove it from the list of clients
      wavelength.clients.delete(ws);

      // Notify the host that the client has been disconnected
      if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
        wavelength.host.send(
          JSON.stringify({
            type: "client_disconnected",
            sessionId: ws.sessionId,
          })
        );
      }
    }
  }

  /**
   * Registers a new wavelength
   * @param {WebSocket} ws - WebSocket instance of the host
   * @param {number} frequency - Frequency of the wavelength
   * @param {string} name - Name of the wavelength (handled automatically by the server)
   * @param {boolean} isPasswordProtected - Whether the wavelength is password protected
   * @param {string} sessionId - session ID
   * @returns {Object} - Information about the registered wavelength
   */
  registerWavelength(ws, frequency, name, isPasswordProtected, sessionId) {
    ws.frequency = frequency;
    ws.isHost = true;
    ws.sessionId = sessionId;

    this.activeWavelengths.set(frequency, {
      host: ws,
      clients: new Set(),
      name,
      isPasswordProtected,
      processedMessageIds: new Set(), // To track processed messages (without their content)
    });

    // Update last recorded frequency (for quicker future searches)
    if (frequency > this.lastRegisteredFrequency) {
      this.lastRegisteredFrequency = frequency;
    }

    return { frequency, sessionId };
  }

  /**
   * Adds client to existing wavelength
   * @param {WebSocket} ws - WebSocket instance of the client
   * @param {number} frequency - Frequency of the wavelength
   * @param {string} sessionId - session ID (client)
   * @returns {Object} - Information about the added client
   * @returns {null} - If wavelength does not exist
   */
  addClientToWavelength(ws, frequency, sessionId) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) return null;

    ws.frequency = frequency;
    ws.isHost = false;
    ws.sessionId = sessionId;

    wavelength.clients.add(ws);

    return {
      frequency,
      name: wavelength.name,
      sessionId,
    };
  }

  /**
   * Deletes wavelength from memory and database
   * @param {number} frequency - Frequency of the wavelength to be removed
   * @returns {boolean} - Whether the wavelength was successfully removed
   * @returns {false} - If wavelength does not exist
   */
  async removeWavelength(frequency) {
    if (!this.activeWavelengths.has(frequency)) return false;

    // Memory cleanup
    this.activeWavelengths.delete(frequency);

    // Database cleanup
    try {
      await WavelengthModel.delete(frequency);
      return true;
    } catch (error) {
      console.error(
        `Error removing wavelength ${frequency} from database:`,
        error
      );
      return false;
    }
  }

  /**
   * Retrieves wavelength from memory or database
   * @param {number} frequency - Frequency of the wavelength
   * @returns {Promise<Object|null>} - Wavelength information or null if not found
   */
  async getWavelength(frequency) {
    // Memory check
    if (this.activeWavelengths.has(frequency)) {
      return this.activeWavelengths.get(frequency);
    }

    // Database check
    try {
      return await WavelengthModel.findByFrequency(frequency);
    } catch (error) {
      console.error(
        `Error finding wavelength ${frequency} in database:`,
        error
      );
      return null;
    }
  }

  /**
   * Checks if the message has already been processed
   * @param {number} frequency - Frequency of the wavelength
   * @param {string} messageId - ID of the message
   * @returns {boolean} - Whether the message has been processed
   */
  isMessageProcessed(frequency, messageId) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) return false;

    return wavelength.processedMessageIds.has(messageId);
  }

  /**
   * Marks the message as processed
   * @param {number} frequency - Frequency of the wavelength
   * @param {string} messageId - ID of the message
   */
  markMessageProcessed(frequency, messageId) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) return;

    wavelength.processedMessageIds.add(messageId);

    // Limiting size of the collection
    if (wavelength.processedMessageIds.size > 1000) {
      // Remove the oldest elements (the first 200)
      const iterator = wavelength.processedMessageIds.values();
      for (let i = 0; i < 200; i++) {
        wavelength.processedMessageIds.delete(iterator.next().value);
      }
    }
  }
}

module.exports = new ConnectionManager();
