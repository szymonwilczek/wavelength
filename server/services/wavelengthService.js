const WavelengthModel = require("../models/wavelength");
const connectionManager = require("../websocket/connectionManager");
const { normalizeFrequency, generateSessionId } = require("../utils/helpers");

/**
 * Wavelength service - logic handler for wavelengths
 */
class WavelengthService {
  /**
   * Registers a new wavelength in the system
   * @param {WebSocket} ws - Host WebSocket connection
   * @param {number} frequency - Frequency to register
   * @param {string} name - Name of the wavelength (handled automatically by the server)
   * @param {boolean} isPasswordProtected - Is the wavelength password protected?
   * @returns {Promise<Object>} - Object with the result of the registration
   * @throws {Error} - Throws an error if the registration fails
   */
  async registerWavelength(ws, frequency, name, isPasswordProtected) {
    const normalizedFrequency = normalizeFrequency(frequency);

    if (connectionManager.activeWavelengths.has(normalizedFrequency)) {
      return {
        success: false,
        error: "Frequency is already in use",
      };
    }

    try {
      const existingWavelength = await WavelengthModel.findByFrequency(
        normalizedFrequency
      );

      if (existingWavelength) {
        await WavelengthModel.delete(normalizedFrequency);
      }

      const sessionId = generateSessionId("ws");

      await WavelengthModel.create(
        normalizedFrequency,
        name,
        isPasswordProtected,
        sessionId
      );

      connectionManager.registerWavelength(
        ws,
        normalizedFrequency,
        name,
        isPasswordProtected,
        sessionId
      );

      return {
        success: true,
        frequency: normalizedFrequency,
        sessionId,
      };
    } catch (error) {
      console.error(
        `Error registering wavelength ${normalizedFrequency}:`,
        error
      );
      return {
        success: false,
        error: "Server error, please try again",
      };
    }
  }

  /**
   * Adds client to wavelength
   * @param {WebSocket} ws - Client WebSocket connection
   * @param {number} frequency - Frequency to join
   * @param {string} password - Password for the wavelength (if required)
   * @returns {Promise<Object>} - Object with the result of the join attempt
   * @throws {Error} - Throws an error if the join attempt fails
   */
  async joinWavelength(ws, frequency, password) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const wavelength =
      connectionManager.activeWavelengths.get(normalizedFrequency);

    if (!wavelength) {
      try {
        const dbWavelength = await WavelengthModel.findByFrequency(
          normalizedFrequency
        );

        if (!dbWavelength) {
          return {
            success: false,
            error: "Wavelength does not exist",
          };
        }

        return {
          success: false,
          error: "Wavelength host is offline",
        };
      } catch (error) {
        console.error(
          `Error checking wavelength ${normalizedFrequency} in database:`,
          error
        );
        return {
          success: false,
          error: "Database error",
        };
      }
    }

    if (wavelength.isPasswordProtected && password !== wavelength.password) {
      return {
        success: false,
        error: "Invalid password",
      };
    }

    const sessionId = generateSessionId("client");

    connectionManager.addClientToWavelength(ws, normalizedFrequency, sessionId);

    return {
      success: true,
      frequency: normalizedFrequency,
      name: wavelength.name,
      sessionId,
    };
  }

  /**
   * Closes wavelength and notifies clients
   * @param {number} frequency - Frequency to close
   * @param {string} reason - Reason for closing the wavelength
   * @returns {Promise<boolean>} - True if the wavelength was closed successfully, false otherwise
   * @throws {Error} - Throws an error if the closing fails
   */
  async closeWavelength(frequency, reason) {
    const normalizedFrequency = normalizeFrequency(frequency);

    try {
      await WavelengthModel.delete(normalizedFrequency);

      if (connectionManager.activeWavelengths.has(normalizedFrequency)) {
        connectionManager.activeWavelengths.delete(normalizedFrequency);
      }

      return true;
    } catch (error) {
      console.error(`Error closing wavelength ${normalizedFrequency}:`, error);
      return false;
    }
  }

  /**
   * Finds the next available frequency
   * @param {number} startFrequency - Initial frequency
   * @param {number} increment - Increment value for frequency search (default is 0.1)
   * @returns {Promise<number|null>} - Next available frequency or null if not found
   * @throws {Error} - Throws an error if the search fails
   */
  async findNextAvailableFrequency(startFrequency = 130.0, increment = 0.1) {
    let nextFreq = normalizeFrequency(startFrequency);
    let safetyCounter = 0;

    while (safetyCounter < 1000) {
      nextFreq = normalizeFrequency(nextFreq + increment);

      if (connectionManager.activeWavelengths.has(nextFreq)) {
        safetyCounter++;
        continue;
      }

      try {
        const exists = await WavelengthModel.findByFrequency(nextFreq);
        if (!exists) {
          return nextFreq;
        }
      } catch (error) {
        console.error(`Error checking frequency ${nextFreq}:`, error);
      }

      safetyCounter++;
    }

    return null;
  }

  /**
   * Retrieves all active wavelengths
   * @returns {Promise<Array>} - Array of active wavelengths
   * @throws {Error} - Throws an error if the retrieval fails
   */
  async getAllWavelengths() {
    try {
      return await WavelengthModel.findAll();
    } catch (error) {
      console.error("Error getting all wavelengths:", error);
      return [];
    }
  }

  /**
   * Initializes the database by clearing outdated data
   * @returns {Promise<void>} - Promise that resolves when the database is initialized
   */
  async initializeDatabase() {
    try {
      await WavelengthModel.deleteAll();
      console.log("Database initialized and cleared");
    } catch (error) {
      console.error("Error initializing database:", error);
      throw error;
    }
  }
}

module.exports = new WavelengthService();
