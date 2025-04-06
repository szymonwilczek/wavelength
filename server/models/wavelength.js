/**
 * Model Wavelength - operacje na bazie danych dla wavelengths
 */
const { query } = require("../config/db");
const { normalizeFrequency } = require("../utils/helpers");

class WavelengthModel {
  /**
   * Tworzy nowy wavelength w bazie danych
   * @param {number} frequency - Częstotliwość
   * @param {string} name - Nazwa wavelength
   * @param {boolean} isPasswordProtected - Czy jest zabezpieczony hasłem
   * @param {string} hostSocketId - ID gniazda hosta
   * @returns {Promise<Object>} - Utworzony wavelength
   */
  static async create(frequency, name, isPasswordProtected, hostSocketId) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const result = await query(
      "INSERT INTO active_wavelengths (frequency, name, is_password_protected, host_socket_id, created_at) VALUES ($1, $2, $3, $4, $5) RETURNING *",
      [normalizedFrequency, name, isPasswordProtected, hostSocketId, new Date()]
    );
    return result.rows[0];
  }

  /**
   * Wyszukuje wavelength po częstotliwości
   * @param {number} frequency - Częstotliwość do wyszukania
   * @returns {Promise<Object|null>} - Znaleziony wavelength lub null
   */
  static async findByFrequency(frequency) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const result = await query(
      "SELECT * FROM active_wavelengths WHERE frequency = $1",
      [normalizedFrequency]
    );
    return result.rows.length > 0 ? result.rows[0] : null;
  }

  /**
   * Pobiera wszystkie aktywne wavelengths
   * @returns {Promise<Array>} - Lista aktywnych wavelengths
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
   * Usuwa wavelength o podanej częstotliwości
   * @param {number} frequency - Częstotliwość do usunięcia
   * @returns {Promise<boolean>} - Czy udało się usunąć
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
   * Usuwa wszystkie wavelengths z bazy danych
   * @returns {Promise<void>}
   */
  static async deleteAll() {
    await query("DELETE FROM active_wavelengths");
  }

  /**
   * Znajduje następną dostępną częstotliwość
   * @param {number} startFrequency - Częstotliwość startowa
   * @param {number} increment - Przyrost częstotliwości
   * @returns {Promise<number|null>} - Znaleziona częstotliwość lub null
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
