/**
 * Normalizes the frequency value to a string with one decimal place.
 * @param {number|string} frequency - Frequency to be normalized.
 * @returns {string} - Normalized frequency string (e.g., "130.0").
 * @throws {TypeError} - Throws an error if the frequency cannot be parsed to a valid positive number.
 */
function normalizeFrequency(frequency) {
  const numFreq = typeof frequency === 'string' ? parseFloat(frequency.trim()) : frequency;
  if (isNaN(numFreq) || numFreq <= 0) { 
    throw new TypeError(`Invalid frequency value provided: ${frequency}`);
  }
  return numFreq.toFixed(1);
}

/**
 * Generates a unique session ID
 * @param {string} prefix - Prefix for the identifier (e.g. ‘ws_’ for host, ‘client_’ for client)
 * @returns {string} - Unique session ID
 */
function generateSessionId(prefix = "ws") {
  return `${prefix}_${Date.now()}_${Math.floor(Math.random() * 1000)}`;
}

/**
 * Generates a unique message ID
 * @param {string} prefix - Prefix for the identifier (e.g. ‘msg_’ for messages, ‘file_’ for files)
 * @returns {string} - Unique message ID
 */
function generateMessageId(prefix = "msg") {
  return `${prefix}_${Date.now()}_${Math.floor(Math.random() * 10000)}`;
}

/**
 * Checks if the input can be normalized to a valid frequency string.
 * @param {number|string} frequency - Frequency to be checked.
 * @returns {boolean} - True if the frequency is valid, false otherwise.
 */
function isValidFrequency(frequency) {
  try {
    normalizeFrequency(frequency); 
    return true;
  } catch (e) {
    return false; 
  }
}

module.exports = {
  normalizeFrequency,
  generateSessionId,
  generateMessageId,
  isValidFrequency,
};
