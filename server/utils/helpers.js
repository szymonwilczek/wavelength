/**
 * Normalizuje wartość częstotliwości do liczby z jednym miejscem po przecinku.
 * @param {number|string} frequency - Częstotliwość do znormalizowania
 * @returns {number} - Znormalizowana częstotliwość
 */
function normalizeFrequency(frequency) {
  // Konwersja do liczby jeśli to string
  const numFreq =
    typeof frequency === "string" ? parseFloat(frequency) : frequency;
  // Zaokrąglenie do 1 miejsca po przecinku i konwersja z powrotem do liczby
  return parseFloat(numFreq.toFixed(1));
}

/**
 * Generuje unikalny identyfikator sesji
 * @param {string} prefix - Prefiks dla identyfikatora (np. 'ws_' dla hosta, 'client_' dla klienta)
 * @returns {string} - Unikalny identyfikator sesji
 */
function generateSessionId(prefix = "ws") {
  return `${prefix}_${Date.now()}_${Math.floor(Math.random() * 1000)}`;
}

/**
 * Generuje unikalny identyfikator wiadomości
 * @param {string} prefix - Prefiks dla identyfikatora (np. 'msg_' dla wiadomości, 'file_' dla plików)
 * @returns {string} - Unikalny identyfikator wiadomości
 */
function generateMessageId(prefix = "msg") {
  return `${prefix}_${Date.now()}_${Math.floor(Math.random() * 10000)}`;
}

/**
 * Sprawdza czy częstotliwość jest prawidłowa
 * @param {number|string} frequency - Częstotliwość do sprawdzenia
 * @returns {boolean} - Czy częstotliwość jest prawidłowa
 */
function isValidFrequency(frequency) {
  const numFreq =
    typeof frequency === "string" ? parseFloat(frequency) : frequency;
  return !isNaN(numFreq) && numFreq > 0;
}

module.exports = {
  normalizeFrequency,
  generateSessionId,
  generateMessageId,
  isValidFrequency,
};
