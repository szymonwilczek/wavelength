// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\models\wavelength.js
/**
 * Wavelength model - database related operations for wavelengths
 */
const { query } = require("../config/db");
const { normalizeFrequency } = require("../utils/helpers"); // Assuming normalizeFrequency now returns string
const crypto = require('crypto');

class WavelengthModel {
  /**
   * Creates a new wavelength in the database
   * @param {string} frequency - Frequency string of the wavelength
   * @param {string} name - Name of the wavelength
   * @param {boolean} isPasswordProtected - Is the wavelength password protected?
   * @param {string} hostSocketId - Socket ID of the host (using session ID)
   * @param {string|null} password - Raw password for the wavelength (if applicable), null if not protected
   * @returns {Promise<Object>} - Created wavelength object (frequency as string)
   * @throws {Error} - Throws an error if the creation fails
   */
  static async create(frequency, name, isPasswordProtected, hostSocketId, password) {
    const normalizedFrequency = normalizeFrequency(frequency); // Ensures string format "XXX.X"
    const hashedPassword = (isPasswordProtected && password) ?
        crypto.createHash('sha256').update(password).digest('hex') :
        null;
    const result = await query(
        `INSERT INTO active_wavelengths
         (frequency, name, is_password_protected, host_socket_id, password_hash, created_at)
         VALUES ($1, $2, $3, $4, $5, $6)
           RETURNING id, frequency, name, is_password_protected, host_socket_id, password_hash, created_at`, // frequency is TEXT
        [normalizedFrequency, name, isPasswordProtected, hostSocketId, hashedPassword, new Date()]
    );
    // Ensure returned frequency is also treated as string
    if (result.rows[0]) {
      result.rows[0].frequency = String(result.rows[0].frequency);
    }
    return result.rows[0];
  }

  /**
   * Searches for wavelength by frequency string
   * @param {string} frequency - Frequency string to be searched
   * @returns {Promise<Object|null>} - Found wavelength object (frequency as string) or null
   */
  static async findByFrequency(frequency) {
    const normalizedFrequency = normalizeFrequency(frequency); // Ensures string format "XXX.X"
    const result = await query(
        `SELECT
           id, frequency, name, is_password_protected, host_socket_id, password_hash, created_at
         FROM active_wavelengths
         WHERE frequency = $1`, // frequency is TEXT
        [normalizedFrequency]
    );
    const row = result.rows.length > 0 ? result.rows[0] : null;
    if (row) {
      row.frequency = String(row.frequency); // Ensure string type
      // Map db column names to consistent camelCase if needed by consumers
      row.isPasswordProtected = row.is_password_protected;
      row.hostSocketId = row.host_socket_id;
      row.passwordHash = row.password_hash;
      row.createdAt = row.created_at;
    }
    return row;
  }

  /**
   * Retrieves all active wavelengths from the database
   * @returns {Promise<Array>} - List of active wavelength objects (frequency as string)
   */
  static async findAll() {
    const result = await query(`
      SELECT
        id, frequency, name, is_password_protected, host_socket_id, password_hash, created_at
      FROM active_wavelengths
      ORDER BY created_at DESC
    `);
    // Ensure frequency is string and map columns
    return result.rows.map(row => ({
      id: row.id,
      frequency: String(row.frequency),
      name: row.name,
      isPasswordProtected: row.is_password_protected,
      hostSocketId: row.host_socket_id,
      passwordHash: row.password_hash,
      createdAt: row.created_at
    }));
  }

  /**
   * Removes the wavelength of the specified frequency string from the database
   * @param {string} frequency - Frequency string to be removed
   * @returns {Promise<boolean>} - True if the wavelength was successfully removed, false otherwise
   * @throws {Error} - Throws an error if the deletion fails
   */
  static async delete(frequency) {
    const normalizedFrequency = normalizeFrequency(frequency); // Ensures string format "XXX.X"
    const result = await query(
        "DELETE FROM active_wavelengths WHERE frequency = $1", // frequency is TEXT
        [normalizedFrequency]
    );
    return result.rowCount > 0;
  }

  /**
   * Deletes all wavelengths from the database
   * @returns {Promise<void>}
   */
  static async deleteAll() {
    await query("DELETE FROM active_wavelengths");
  }

  /**
   * Finds the next available frequency string in the database (less efficient than tracker)
   * Primarily for completeness or fallback. Use FrequencyGapTracker for finding next available.
   * @param {string} startFrequency - Start frequency string
   * @param {number} increment - Frequency increment (default is 0.1)
   * @returns {Promise<string|null>} - Next available frequency string or null
   */
  static async findNextAvailableFrequency(startFrequency, increment = 0.1) {
    console.warn("Using WavelengthModel.findNextAvailableFrequency - consider using FrequencyGapTracker instead for performance.");
    let currentFreqNum = parseFloat(normalizeFrequency(startFrequency));
    let safetyCounter = 0;
    const maxCounter = 10000; // Increased safety limit

    while (safetyCounter < maxCounter) {
      currentFreqNum += increment;
      // Ensure precision issues don't skip values
      currentFreqNum = parseFloat(currentFreqNum.toFixed(1));
      const nextFreqStr = currentFreqNum.toFixed(1);

      // Check bounds (example: stop searching above a certain limit)
      // if (currentFreqNum > 1000.0) { break; }

      const existingResult = await query(
          "SELECT 1 FROM active_wavelengths WHERE frequency = $1 LIMIT 1", // Use SELECT 1 for existence check
          [nextFreqStr]
      );

      if (existingResult.rows.length === 0) {
        return nextFreqStr; // Found available
      }

      safetyCounter++;
    }
    console.error(`findNextAvailableFrequency exceeded safety limit (${maxCounter}) starting from ${startFrequency}`);
    return null; // Not found within safety limit
  }
}

module.exports = WavelengthModel;