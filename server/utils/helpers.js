/**
 * Normalizes the frequency value to a number with one decimal place.
 * @param {number|string} frequency - Frequency to be normalized, can be a number or a string.
 * @returns {number} - Normalized frequency value rounded to one decimal place.
 * @throws {TypeError} - Throws an error if the frequency is not a number or string.
 */
function normalizeFrequency(frequency) {
  const numFreq =
    typeof frequency === "string" ? parseFloat(frequency) : frequency;
  return parseFloat(numFreq.toFixed(1));
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
 * Checks that the frequency is correct
 * @param {number|string} frequency - Frequency to be checked, can be a number or a string.
 * @returns {boolean} - True if the frequency is valid, false otherwise.
 * @throws {TypeError} - Throws an error if the frequency is not a number or string.
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
