/**
 * Usługa wavelength - logika biznesowa dla wavelengths
 */
const WavelengthModel = require("../models/wavelength");
const connectionManager = require("../websocket/connectionManager");
const { normalizeFrequency, generateSessionId } = require("../utils/helpers");

class WavelengthService {
  /**
   * Rejestruje nowy wavelength w systemie
   * @param {WebSocket} ws - WebSocket hosta
   * @param {number} frequency - Częstotliwość
   * @param {string} name - Nazwa
   * @param {boolean} isPasswordProtected - Czy jest zabezpieczony hasłem
   * @returns {Promise<Object>} - Obiekt z wynikiem rejestracji
   */
  async registerWavelength(ws, frequency, name, isPasswordProtected) {
    const normalizedFrequency = normalizeFrequency(frequency);

    // Sprawdź czy częstotliwość jest już zajęta w pamięci
    if (connectionManager.activeWavelengths.has(normalizedFrequency)) {
      return {
        success: false,
        error: "Frequency is already in use",
      };
    }

    try {
      // Sprawdź czy częstotliwość istnieje w bazie danych
      const existingWavelength = await WavelengthModel.findByFrequency(
        normalizedFrequency
      );

      // Jeśli istnieje, usuń ją
      if (existingWavelength) {
        await WavelengthModel.delete(normalizedFrequency);
      }

      // Generuj ID sesji
      const sessionId = generateSessionId("ws");

      // Zapisz do bazy danych
      await WavelengthModel.create(
        normalizedFrequency,
        name,
        isPasswordProtected,
        sessionId
      );

      // Zarejestruj w menedżerze połączeń
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
   * Dołącza klienta do wavelength
   * @param {WebSocket} ws - WebSocket klienta
   * @param {number} frequency - Częstotliwość
   * @param {string} password - Hasło (jeśli chroniony)
   * @returns {Promise<Object>} - Obiekt z wynikiem dołączania
   */
  async joinWavelength(ws, frequency, password) {
    const normalizedFrequency = normalizeFrequency(frequency);
    const wavelength =
      connectionManager.activeWavelengths.get(normalizedFrequency);

    // Sprawdź czy wavelength istnieje w pamięci
    if (!wavelength) {
      try {
        // Sprawdź w bazie danych
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

    // Sprawdź hasło
    if (wavelength.isPasswordProtected && password !== password) {
      // Tutaj można dodać rzeczywiste porównanie hasła
      return {
        success: false,
        error: "Invalid password",
      };
    }

    // Generuj ID sesji dla klienta
    const sessionId = generateSessionId("client");

    // Dodaj klienta do wavelength
    connectionManager.addClientToWavelength(ws, normalizedFrequency, sessionId);

    return {
      success: true,
      frequency: normalizedFrequency,
      name: wavelength.name,
      sessionId,
    };
  }

  /**
   * Zamyka wavelength i powiadamia klientów
   * @param {number} frequency - Częstotliwość
   * @param {string} reason - Powód zamknięcia
   * @returns {Promise<boolean>} - Czy udało się zamknąć
   */
  async closeWavelength(frequency, reason) {
    const normalizedFrequency = normalizeFrequency(frequency);

    try {
      // Usuń z bazy danych
      await WavelengthModel.delete(normalizedFrequency);

      // Usuń z pamięci
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
   * Znajduje następną dostępną częstotliwość
   * @param {number} startFrequency - Częstotliwość początkowa
   * @param {number} increment - Przyrost częstotliwości
   * @returns {Promise<number|null>} - Znaleziona częstotliwość lub null
   */
  async findNextAvailableFrequency(startFrequency = 130.0, increment = 0.1) {
    let nextFreq = normalizeFrequency(startFrequency);
    let safetyCounter = 0;

    while (safetyCounter < 1000) {
      nextFreq = normalizeFrequency(nextFreq + increment);

      // Sprawdź w pamięci
      if (connectionManager.activeWavelengths.has(nextFreq)) {
        safetyCounter++;
        continue;
      }

      // Sprawdź w bazie danych
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
   * Pobiera wszystkie aktywne wavelengths
   * @returns {Promise<Array>} - Lista aktywnych wavelengths
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
   * Inicjalizuje bazę danych
   */
  async initializeDatabase() {
    try {
      // Wyczyść nieaktualne dane
      await WavelengthModel.deleteAll();
      console.log("Database initialized and cleared");
    } catch (error) {
      console.error("Error initializing database:", error);
      throw error;
    }
  }
}

module.exports = new WavelengthService();
