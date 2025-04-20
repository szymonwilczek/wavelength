// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\services\wavelengthService.js
const WebSocket = require("ws"); // Required for type hint on 'ws' param
const crypto = require("crypto"); // Required for password hashing if join logic remains here
const WavelengthModel = require("../models/wavelength"); // Uses string frequencies
const connectionManager = require("../websocket/connectionManager"); // Uses string frequencies
const { normalizeFrequency, generateSessionId, isValidFrequency } = require("../utils/helpers"); // Use string helpers
const frequencyTracker = require("./frequencyGapTracker"); // Import tracker

/**
 * Wavelength service - logic handler for wavelengths
 */
class WavelengthService {
  /**
   * Registers a new wavelength in the system
   * @param {WebSocket} ws - Host WebSocket connection
   * @param {string|number} frequencyInput - Raw frequency input from client
   * @param {string} name - Name of the wavelength
   * @param {boolean} isPasswordProtected - Is the wavelength password protected?
   * @param {string|null} password - Raw password for the wavelength (if applicable)
   * @returns {Promise<Object>} - Object with registration result { success: boolean, frequency?: string, sessionId?: string, error?: string }
   */
  async registerWavelength(ws, frequencyInput, name, isPasswordProtected, password) {
    let normalizedFrequency;
    try {
      // Validate and normalize input frequency
      if (!isValidFrequency(frequencyInput)) {
        throw new Error("Invalid frequency format. Must be a positive number.");
      }
      normalizedFrequency = normalizeFrequency(frequencyInput); // Get string "XXX.X"
    } catch (e) {
      console.error(`registerWavelength Service: Invalid frequency input: ${frequencyInput}`, e.message);
      return { success: false, error: e.message };
    }

    console.log(`Service: Attempting registration for frequency ${normalizedFrequency}`);

    // 1. Check if already active in memory
    if (connectionManager.activeWavelengths.has(normalizedFrequency)) {
      console.warn(`Service: Frequency ${normalizedFrequency} is already active in memory.`);
      return { success: false, error: "Frequency is already in use" };
    }

    // 2. Check Frequency Tracker cache
    if (!frequencyTracker.isFrequencyAvailable(normalizedFrequency)) {
      console.warn(`Service: Frequency ${normalizedFrequency} marked unavailable by tracker. Verifying DB.`);
      // Double check DB for potential zombie entries
      const existingDb = await WavelengthModel.findByFrequency(normalizedFrequency);
      if (existingDb) {
        console.log(`Service: Found zombie DB entry for ${normalizedFrequency}. Deleting before registration.`);
        await WavelengthModel.delete(normalizedFrequency);
        // No need to update tracker here, addTakenFrequency below handles it.
      } else {
        // If tracker says unavailable but DB is clear, tracker might be stale.
        // Proceeding might be okay, but log a warning.
        console.warn(`Service: Frequency ${normalizedFrequency} unavailable in tracker but not found in DB. Proceeding with registration.`);
      }
    }
    // If tracker said available, still double-check DB just before creation (belt-and-suspenders)
    else {
      const existingDb = await WavelengthModel.findByFrequency(normalizedFrequency);
      if (existingDb) {
        console.warn(`Service: Frequency ${normalizedFrequency} available in tracker but FOUND in DB! Deleting zombie entry.`);
        await WavelengthModel.delete(normalizedFrequency);
      }
    }


    try {
      // 3. Create in Database
      const sessionId = generateSessionId("ws"); // Generate unique ID for the host session
      await WavelengthModel.create(
          normalizedFrequency,
          name,
          isPasswordProtected,
          sessionId, // Use session ID as the host identifier in DB
          password // Pass raw password (model handles hashing)
      );
      console.log(`Service: Created DB entry for ${normalizedFrequency}`);

      // 4. Add to Frequency Tracker
      await frequencyTracker.addTakenFrequency(normalizedFrequency);
      console.log(`Service: Added ${normalizedFrequency} to frequency tracker.`);

      // 5. Register in Connection Manager (Memory)
      connectionManager.registerWavelength(
          ws,
          normalizedFrequency,
          name,
          isPasswordProtected,
          password, // Pass raw password (manager handles hashing for memory store)
          sessionId
      );
      console.log(`Service: Registered ${normalizedFrequency} in connection manager.`);

      return {
        success: true,
        frequency: normalizedFrequency, // Return normalized string freq
        sessionId,
      };
    } catch (error) {
      console.error(`Service: Error during registration process for ${normalizedFrequency}:`, error);
      // Attempt cleanup if registration failed partially
      console.log(`Service: Attempting cleanup after failed registration for ${normalizedFrequency}...`);
      await frequencyTracker.removeTakenFrequency(normalizedFrequency); // Remove if added to tracker
      connectionManager.removeWavelengthFromMemory(normalizedFrequency); // Remove from memory if added
      // Avoid deleting from DB here, as the DB error might be the root cause.
      return {
        success: false,
        error: "Server error during registration, please try again",
      };
    }
  }

  /**
   * Adds client to wavelength (Service layer - less common, handlers usually manage this)
   * Kept for potential API use cases or different architectures.
   * @param {WebSocket} ws - Client WebSocket connection
   * @param {string|number} frequencyInput - Raw frequency input
   * @param {string} password - Password attempt
   * @returns {Promise<Object>} - Result object { success: boolean, ... }
   */
  async joinWavelength(ws, frequencyInput, password) {
    // This logic is better handled directly in websocket/handlers.js for WebSocket flow.
    // This implementation is just a placeholder showing string usage.
    console.warn("WavelengthService.joinWavelength called - consider using handler logic directly for WebSocket joins.");
    let normalizedFrequency;
    try {
      if (!isValidFrequency(frequencyInput)) throw new Error("Invalid frequency format.");
      normalizedFrequency = normalizeFrequency(frequencyInput);
    } catch (e) { return { success: false, error: e.message }; }

    const wavelength = connectionManager.activeWavelengths.get(normalizedFrequency);

    if (!wavelength) {
      const dbWavelength = await WavelengthModel.findByFrequency(normalizedFrequency);
      return { success: false, error: dbWavelength ? "Wavelength host is offline" : "Wavelength does not exist" };
    }

    if (wavelength.isPasswordProtected) {
      if (!password) return { success: false, error: "Password required" };
      const hashedInput = crypto.createHash('sha256').update(password).digest('hex');
      if (hashedInput !== wavelength.password) return { success: false, error: "Invalid password" };
    }

    const sessionId = generateSessionId("client");
    const result = connectionManager.addClientToWavelength(ws, normalizedFrequency, sessionId);

    if (result) {
      if (wavelength.host && wavelength.host.readyState === WebSocket.OPEN) {
        wavelength.host.send(JSON.stringify({ type: "client_joined", clientId: sessionId, frequency: normalizedFrequency }));
      }
      return { success: true, frequency: result.frequency, name: result.name, sessionId: result.sessionId };
    } else {
      return { success: false, error: "Failed to add client" };
    }
  }

  /**
   * Closes a wavelength and performs all necessary cleanup.
   * @param {string|number} frequencyInput - Raw frequency input
   * @param {string} reason - Reason for closing
   * @returns {Promise<boolean>} - True if cleanup was successful or wavelength wasn't active, false on error during cleanup.
   */
  async closeWavelength(frequencyInput, reason) {
    let normalizedFrequency;
    try {
      if (!isValidFrequency(frequencyInput)) throw new Error("Invalid frequency format.");
      normalizedFrequency = normalizeFrequency(frequencyInput);
    } catch (e) {
      console.error(`closeWavelength Service: Invalid frequency input: ${frequencyInput}`, e.message);
      return false; // Cannot close invalid frequency
    }

    console.log(`Service: Closing wavelength ${normalizedFrequency}. Reason: ${reason}`);
    const wavelength = connectionManager.activeWavelengths.get(normalizedFrequency);

    // 1. Notify clients & disconnect host (if active in memory)
    if (wavelength) {
      console.log(`Service: Notifying clients and host for ${normalizedFrequency}...`);
      const closeMessage = JSON.stringify({ type: "wavelength_closed", frequency: normalizedFrequency, reason });

      // Notify clients
      wavelength.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
          try { client.send(closeMessage); } catch (e) { console.error(`Error sending close msg to client ${client.sessionId}:`, e); }
          // Optionally terminate clients after notification
          // client.terminate();
        }
      });

      // Terminate host connection (triggers handleDisconnect which should prevent double cleanup)
      if (wavelength.host && wavelength.host.readyState !== WebSocket.CLOSED) {
        try {
          console.log(`Service: Terminating host connection for ${normalizedFrequency}`);
          wavelength.host.terminate();
        } catch (e) { console.error(`Error terminating host for ${normalizedFrequency}:`, e); }
      }

      // 2. Remove from memory (ConnectionManager map)
      connectionManager.removeWavelengthFromMemory(normalizedFrequency);
    } else {
      console.log(`Service: Wavelength ${normalizedFrequency} not found in memory during close operation.`);
    }

    // 3. Always attempt DB and Tracker cleanup
    try {
      console.log(`Service: Cleaning up DB and Tracker for ${normalizedFrequency}...`);
      const deletedFromDb = await WavelengthModel.delete(normalizedFrequency);
      await frequencyTracker.removeTakenFrequency(normalizedFrequency);
      console.log(`Service: Cleanup for ${normalizedFrequency} complete. DB delete: ${deletedFromDb}.`);
      return true; // Indicate cleanup attempt was made and likely succeeded
    } catch (error) {
      console.error(`Service: Error during DB/Tracker cleanup for wavelength ${normalizedFrequency}:`, error);
      return false; // Indicate error during cleanup phase
    }
  }

  /**
   * Finds the next available frequency string using the FrequencyGapTracker.
   * @param {string|number|null} [preferredStartFrequencyInput=null] - Optional preferred frequency input.
   * @returns {Promise<string|null>} - Next available frequency string ("XXX.X") or null.
   */
  async findNextAvailableFrequency(preferredStartFrequencyInput = null) {
    console.log(`Service: Finding next available frequency. Preferred start input: ${preferredStartFrequencyInput ?? 'None'}`);
    try {
      // Ensure tracker is initialized
      if (!frequencyTracker.initialized) {
        console.warn("Service: Frequency tracker not initialized. Attempting initialization...");
        await frequencyTracker.initialize();
        if (!frequencyTracker.initialized) {
          throw new Error("Frequency tracker failed to initialize.");
        }
      }

      // Validate and normalize preferred frequency if provided
      let preferredStartStr = null;
      if (preferredStartFrequencyInput !== null) {
        try {
          if (!isValidFrequency(preferredStartFrequencyInput)) throw new Error("Invalid preferred format.");
          const normalizedPref = normalizeFrequency(preferredStartFrequencyInput);
          if (parseFloat(normalizedPref) >= 130.0) {
            preferredStartStr = normalizedPref; // Use normalized string "XXX.X"
            console.log(`Service: Using normalized preferred start: ${preferredStartStr}`);
          } else {
            console.warn(`Service: Preferred frequency ${normalizedPref} is below 130.0. Ignoring.`);
          }
        } catch (e) {
          console.warn(`Service: Invalid preferred frequency format: ${preferredStartFrequencyInput}. Ignoring preference.`, e.message);
        }
      }

      // Use the tracker to find the lowest available frequency (returns number or null)
      const nextFrequencyNum = frequencyTracker.findLowestAvailableFrequency(preferredStartStr);

      if (nextFrequencyNum !== null) {
        // Normalize the number back to string "XXX.X" for return
        const nextFrequencyStr = normalizeFrequency(nextFrequencyNum);
        console.log(`Service: Found next available frequency: ${nextFrequencyStr}`);
        return nextFrequencyStr;
      } else {
        console.log("Service: No available frequencies found by tracker.");
        return null;
      }
    } catch (error) {
      console.error("Service: Error finding next available frequency:", error);
      return null; // Return null on error
    }
  }


  /**
   * Retrieves all active wavelengths (from DB).
   * @returns {Promise<Array<Object>>} - Array of wavelength objects (frequency as string).
   */
  async getAllWavelengths() {
    console.log("Service: Getting all wavelengths from DB...");
    try {
      // WavelengthModel.findAll already returns frequency as string and maps columns
      const wavelengths = await WavelengthModel.findAll();
      console.log(`Service: Found ${wavelengths.length} wavelengths in DB.`);
      return wavelengths;
    } catch (error) {
      console.error("Service: Error getting all wavelengths:", error);
      return []; // Return empty array on error
    }
  }

  /**
   * Initializes the database by clearing all wavelength data and resetting the tracker.
   * WARNING: This deletes all existing wavelength data.
   * @returns {Promise<void>}
   */
  async initializeDatabase() {
    try {
      console.warn("Service: INITIALIZING DATABASE - DELETING ALL WAVELENGTHS!");
      await WavelengthModel.deleteAll();
      console.log("Service: Database cleared.");

      console.log("Service: Re-initializing frequency tracker...");
      frequencyTracker.gaps.clear();
      frequencyTracker.takenFrequencies.clear();
      frequencyTracker.initialized = false; // Mark as not initialized
      await frequencyTracker.initialize(); // Re-run initialization based on empty DB
      console.log("Service: Database and tracker initialization complete.");
    } catch (error) {
      console.error("Service: Error initializing database/tracker:", error);
      throw error; // Re-throw error to indicate failure at app start
    }
  }

  /**
   * Retrieves wavelength info by frequency string (checks memory then DB).
   * @param {string|number} frequencyInput - Raw frequency input.
   * @returns {Promise<Object|null>} - Wavelength object (frequency as string, includes isOnline flag) or null.
   */
  async getWavelength(frequencyInput) {
    let normalizedFrequency;
    try {
      if (!isValidFrequency(frequencyInput)) throw new Error("Invalid frequency format.");
      normalizedFrequency = normalizeFrequency(frequencyInput);
    } catch (e) {
      console.error(`getWavelength Service: Invalid frequency input: ${frequencyInput}`, e.message);
      return null;
    }

    console.log(`Service: Getting wavelength info for ${normalizedFrequency}...`);
    try {
      // Use connectionManager's getWavelength which checks memory then DB
      const wavelengthInfo = await connectionManager.getWavelength(normalizedFrequency);
      if (wavelengthInfo) {
        console.log(`Service: Found wavelength ${normalizedFrequency}. Online: ${wavelengthInfo.isOnline}`);
      } else {
        console.log(`Service: Wavelength ${normalizedFrequency} not found.`);
      }
      return wavelengthInfo; // Returns object with frequency as string and isOnline flag, or null
    } catch (error) {
      console.error(`Service: Error getting wavelength ${normalizedFrequency}:`, error);
      return null; // Return null on error
    }
  }
}

module.exports = new WavelengthService();