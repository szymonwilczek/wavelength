/**
 * Wavelength model - database related operations for wavelengths
 */
const { query } = require("../config/db");
const { normalizeFrequency } = require("../utils/helpers");
const crypto = require('crypto');

class WavelengthModel {
  /**
   * Creates a new wavelength in the database
   * @param {number} frequency - Frequency of the wavelength
   * @param {string} name - Name of the wavelength (mostly handled automatically by the server)
   * @param {boolean} isPasswordProtected - Is the wavelength password protected?
   * @param {string} hostSocketId - Socket ID of the host
   * @param {string} password - Password for the wavelength (if applicable)
   * @returns {Promise<Object>} - Created wavelength object
   * @throws {Error} - Throws an error if the creation fails
   */
  static async create(frequency, name, isPasswordProtected, hostSocketId, password) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const hashedPassword = isPasswordProtected ?
        crypto.createHash('sha256').update(password).digest('hex') :
        null;
    const result = await query(
        `INSERT INTO active_wavelengths 
    (frequency, name, is_password_protected, host_socket_id, password_hash, created_at) 
    VALUES ($1, $2, $3, $4, $5, $6) 
    RETURNING *`,
        [normalizedFrequency, name, isPasswordProtected, hostSocketId, hashedPassword, new Date()]
    );
    return result.rows[0];
  }

  /**
   * Searches for wavelength by frequency
   * @param {number} frequency - Frequency to be searched
   * @returns {Promise<Object|null>} - Found wavelength or null (if not found)
   */
  static async findByFrequency(frequency) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const result = await query(
        `SELECT 
      frequency, 
      name, 
      is_password_protected,
      host_socket_id,
      password_hash,
      created_at 
    FROM active_wavelengths 
    WHERE frequency = $1`,
        [normalizedFrequency]
    );
    return result.rows.length > 0 ? result.rows[0] : null;
  }

  /**
   * Retrieves all active wavelengths from the database
   * @returns {Promise<Array>} - List of active wavelengths
   */
  static async findAll() {
    const result = await query(`
      SELECT 
        frequency, 
        name, 
        is_password_protected AS "isPasswordProtected", 
        created_at AS "createdAt" 
      FROM active_wavelengths
    `);
    return result.rows;
  }

  /**
   * Removes the wavelength of the specified frequency from the database
   * @param {number} frequency - Frequency to be removed
   * @returns {Promise<boolean>} - True if the wavelength was successfully removed, false otherwise
   * @throws {Error} - Throws an error if the deletion fails
   */
  static async delete(frequency) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const result = await query(
      "DELETE FROM active_wavelengths WHERE frequency = $1",
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
   * Finds the next available frequency in the database
   * @param {number} startFrequency - Start frequencyStart frequency (from it the algorithm starts the search)
   * @param {number} increment - Frequency increment (default is 0.1 for every Hz/MHz/GHz)
   * @returns {Promise<number|null>} - Next available frequency or null (if not found)
   * @throws {Error} - Throws an error if the search fails
   */
  static async findNextAvailableFrequency(startFrequency, increment = 0.1) {
    let nextFreq = normalizeFrequency(startFrequency);
    let foundAvailable = false;
    let safetyCounter = 0;

    while (!foundAvailable && safetyCounter < 1000) {
      nextFreq = normalizeFrequency(nextFreq + increment);

      const existingResult = await query(
        "SELECT frequency FROM active_wavelengths WHERE frequency = $1",
        [nextFreq]
      );

      if (existingResult.rows.length === 0) {
        foundAvailable = true;
        break;
      }

      safetyCounter++;
    }

    return foundAvailable ? nextFreq : null;
  }
}

module.exports = WavelengthModel;
