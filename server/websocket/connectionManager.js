const WebSocket = require("ws");
const WavelengthModel = require("../models/wavelength");

/**
 * Klasa zarządzająca połączeniami WebSocket i stanem aplikacji
 */
class ConnectionManager {
  constructor() {
    // Mapa przechowująca aktywne częstotliwości i informacje o nich
    this.activeWavelengths = new Map();

    // Przechowuje ostatnią zarejestrowaną częstotliwość
    this.lastRegisteredFrequency = 130.0;

    // Interwał pingowania (ms)
    this.pingInterval = 30000;

    // Inicjalizacja interwału pingowania
    this.heartbeatInterval = null;
  }

  /**
   * Inicjuje pingowanie klientów aby utrzymać połączenia
   * @param {WebSocket.Server} wss - Serwer WebSocket
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
   * Zatrzymuje pingowanie klientów
   */
  stopHeartbeat() {
    clearInterval(this.heartbeatInterval);
  }

  /**
   * Obsługuje rozłączenie klienta
   * @param {WebSocket} ws - WebSocket klienta
   */
  handleDisconnect(ws) {
    if (!ws.frequency) return;

    console.log(`Client disconnected from frequency ${ws.frequency}`);

    const wavelength = this.activeWavelengths.get(ws.frequency);
    if (!wavelength) return;

    // Jeśli to host, usuń cały wavelength
    if (ws.isHost) {
      console.log(
        `Host disconnected from wavelength ${ws.frequency}, removing wavelength`
      );

      // Powiadom klientów o usunięciu wavelength
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
      // Jeśli to klient, usuń go z listy klientów
      wavelength.clients.delete(ws);

      // Powiadom hosta o odłączeniu klienta
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
   * Rejestruje nowy wavelength
   * @param {WebSocket} ws - WebSocket hosta
   * @param {number} frequency - Częstotliwość
   * @param {string} name - Nazwa wavelength
   * @param {boolean} isPasswordProtected - Czy jest zabezpieczony hasłem
   * @param {string} sessionId - ID sesji
   * @returns {Object} - Informacje o zarejestrowanym wavelength
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
      processedMessageIds: new Set(), // Do śledzenia przetworzonych wiadomości
    });

    // Aktualizuj ostatnią zarejestrowaną częstotliwość
    if (frequency > this.lastRegisteredFrequency) {
      this.lastRegisteredFrequency = frequency;
    }

    return { frequency, sessionId };
  }

  /**
   * Dodaje klienta do istniejącego wavelength
   * @param {WebSocket} ws - WebSocket klienta
   * @param {number} frequency - Częstotliwość
   * @param {string} sessionId - ID sesji klienta
   * @returns {Object} - Informacje o dołączonym wavelength
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
   * Usuwa wavelength
   * @param {number} frequency - Częstotliwość wavelength do usunięcia
   * @returns {boolean} - Czy udało się usunąć
   */
  async removeWavelength(frequency) {
    if (!this.activeWavelengths.has(frequency)) return false;

    // Usuń z pamięci
    this.activeWavelengths.delete(frequency);

    // Usuń z bazy danych
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
   * Pobiera wavelength z pamięci lub bazy danych
   * @param {number} frequency - Częstotliwość
   * @returns {Promise<Object|null>} - Znaleziony wavelength lub null
   */
  async getWavelength(frequency) {
    // Sprawdź w pamięci
    if (this.activeWavelengths.has(frequency)) {
      return this.activeWavelengths.get(frequency);
    }

    // Sprawdź w bazie danych
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
   * Sprawdza czy wiadomość była już przetworzona
   * @param {number} frequency - Częstotliwość
   * @param {string} messageId - ID wiadomości
   * @returns {boolean} - Czy wiadomość była przetworzona
   */
  isMessageProcessed(frequency, messageId) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) return false;

    return wavelength.processedMessageIds.has(messageId);
  }

  /**
   * Oznacza wiadomość jako przetworzoną
   * @param {number} frequency - Częstotliwość
   * @param {string} messageId - ID wiadomości
   */
  markMessageProcessed(frequency, messageId) {
    const wavelength = this.activeWavelengths.get(frequency);
    if (!wavelength) return;

    wavelength.processedMessageIds.add(messageId);

    // Ogranicz rozmiar zbioru
    if (wavelength.processedMessageIds.size > 1000) {
      // Usuń najstarsze elementy (pierwszych 200)
      const iterator = wavelength.processedMessageIds.values();
      for (let i = 0; i < 200; i++) {
        wavelength.processedMessageIds.delete(iterator.next().value);
      }
    }
  }
}

module.exports = new ConnectionManager();
