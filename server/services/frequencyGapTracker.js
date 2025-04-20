// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\services\frequencyGapTracker.js
const WavelengthModel = require('../models/wavelength'); // Uses string frequencies
const { normalizeFrequency, isValidFrequency } = require('../utils/helpers'); // Returns string "XXX.X"

/**
 * Class responsible for tracking gaps in frequencies (using string representation "XXX.X")
 */
class FrequencyGapTracker {
    constructor() {
        // Gaps store [startGapString, endGapStringOrInfinity)
        this.gaps = new Map();
        this.initialized = false;
        // Stores taken frequencies as strings "XXX.X"
        this.takenFrequencies = new Set();
        this.minFreq = 130.0; // Minimum allowed frequency (numeric for comparison/calculation)
        this.minFreqStr = this.minFreq.toFixed(1); // "130.0"
        this.step = 0.1; // Numeric step
        this._updatePromise = null; // To prevent concurrent updates
    }

    /**
     * Initializes the FrequencyGapTracker by updating the gaps cache.
     * Ensures only one initialization runs at a time.
     * @returns {Promise<boolean>} - True if initialization was successful, false otherwise.
     */
    async initialize() {
        if (this.initialized) {
            console.log('FrequencyGapTracker: Already initialized.');
            return true;
        }
        if (this._updatePromise) {
            console.log('FrequencyGapTracker: Initialization already in progress, awaiting result...');
            return await this._updatePromise; // Wait for the ongoing update
        }

        console.log('FrequencyGapTracker: Initializing...');
        this._updatePromise = this._initializeInternal();
        try {
            return await this._updatePromise;
        } finally {
            this._updatePromise = null; // Clear promise once done
        }
    }

    async _initializeInternal() {
        try {
            await this.updateGapsCache();
            this.initialized = true;
            console.log(`FrequencyGapTracker: Initialized successfully. Found ${this.takenFrequencies.size} taken frequencies.`);
            return true;
        } catch (error) {
            console.error('FrequencyGapTracker: Error during initialization:', error);
            this.initialized = false; // Ensure it's marked as not initialized on error
            return false;
        }
    }


    /**
     * Updates the gaps cache by querying the database. Prevents concurrent updates.
     * @returns {Promise<boolean>} - True if the cache was updated successfully, false otherwise.
     */
    async updateGapsCache() {
        if (this._updatePromise) {
            console.log('FrequencyGapTracker: Update already in progress, awaiting result...');
            return await this._updatePromise;
        }
        console.log('FrequencyGapTracker: Updating gaps cache...');
        this._updatePromise = this._updateGapsCacheInternal();
        try {
            return await this._updatePromise;
        } finally {
            this._updatePromise = null; // Clear promise once done
        }
    }

    async _updateGapsCacheInternal() {
        try {
            const wavelengths = await WavelengthModel.findAll(); // Gets objects with frequency as string
            console.log(`FrequencyGapTracker: Found ${wavelengths.length} wavelengths in DB for cache update.`);

            // Normalize just in case, though model should return normalized strings
            const frequencies = wavelengths.map(w => normalizeFrequency(w.frequency));
            this.takenFrequencies = new Set(frequencies);
            // console.log('Taken frequencies (strings):', [...this.takenFrequencies]);

            // Sort taken frequencies numerically for gap calculation
            const sortedFreqsNum = [...this.takenFrequencies]
                .map(f => parseFloat(f))
                .sort((a, b) => a - b);
            // console.log('Sorted taken frequencies (numeric):', sortedFreqsNum);

            this.gaps.clear(); // Clear previous gaps
            let lastTakenNum = this.minFreq - this.step; // Start numerically before the minimum

            // Check gap before the first taken frequency
            if (sortedFreqsNum.length === 0 || sortedFreqsNum[0] > this.minFreq) {
                const endGapNum = sortedFreqsNum.length > 0 ? sortedFreqsNum[0] : Infinity;
                const endGapStr = endGapNum === Infinity ? Infinity : endGapNum.toFixed(1);
                this.gaps.set(this.minFreqStr, endGapStr); // Use string keys/values ("130.0" -> "XXX.X" or Infinity)
                // console.log(`Found gap at beginning: ${this.minFreqStr} - ${endGapStr}`);
            }

            // Find gaps between taken frequencies
            for (const currentTakenNum of sortedFreqsNum) {
                // Skip if currentTaken is the absolute minimum (already handled by the check above)
                if (currentTakenNum <= this.minFreq && this.gaps.has(this.minFreqStr)) {
                    lastTakenNum = currentTakenNum;
                    continue;
                }

                // Calculate next potential start numerically, ensuring precision
                const potentialGapStartNum = parseFloat((lastTakenNum + this.step).toFixed(1));

                if (currentTakenNum > potentialGapStartNum) {
                    const startGapStr = potentialGapStartNum.toFixed(1);
                    const endGapStr = currentTakenNum.toFixed(1);
                    this.gaps.set(startGapStr, endGapStr); // Use string keys/values ("XXX.X" -> "YYY.Y")
                    // console.log(`Found gap: ${startGapStr} - ${endGapStr}`);
                }
                lastTakenNum = currentTakenNum;
            }

            // Add gap after the last taken frequency
            const gapAfterLastStartNum = parseFloat((lastTakenNum + this.step).toFixed(1));
            const gapAfterLastStartStr = gapAfterLastStartNum.toFixed(1);
            this.gaps.set(gapAfterLastStartStr, Infinity); // Use string key ("ZZZ.Z" -> Infinity)
            // console.log(`Found gap after last: ${gapAfterLastStartStr} - Infinity`);

            console.log(`FrequencyGapTracker: Gaps cache updated. ${this.gaps.size} gaps found.`);
            // console.log('Gaps cache:', [...this.gaps.entries()]); // Can be large
            return true;
        } catch (error) {
            console.error('FrequencyGapTracker: Error updating frequency gaps cache:', error);
            return false;
        }
    }

    /**
     * Finds the lowest available frequency (as a number), optionally starting >= a preferred frequency.
     * Note: Returns number for easier comparison/sorting by caller, caller should normalize to string.
     * @param {string|null} [preferredStartFrequencyStr=null] - Preferred frequency string ("XXX.X") >= 130.0.
     * @returns {number|null} - Lowest available frequency as a number (e.g., 130.1) or null.
     */
    findLowestAvailableFrequency(preferredStartFrequencyStr = null) {
        if (!this.initialized) {
            console.error('FrequencyGapTracker: findLowestAvailableFrequency called before initialization.');
            // Attempting sync init here is risky. Throw error or return null.
            // Let's return null and log error.
            return null;
            // throw new Error('FrequencyGapTracker not initialized');
        }

        let searchStartNum = this.minFreq; // Start searching from numeric minimum (130.0)

        // Validate and parse preferred frequency string
        if (preferredStartFrequencyStr !== null) {
            try {
                // Use normalizeFrequency to validate format and get consistent string
                const normalizedPreferredStr = normalizeFrequency(preferredStartFrequencyStr);
                const preferredNum = parseFloat(normalizedPreferredStr);

                if (preferredNum >= this.minFreq) {
                    console.log(`FrequencyGapTracker: Preferred frequency provided: ${normalizedPreferredStr}`);
                    searchStartNum = preferredNum; // Update numeric search start point

                    // Optimization: Check if the exact preferred frequency is available
                    if (!this.takenFrequencies.has(normalizedPreferredStr)) {
                        // Verify it falls within a known gap (sanity check, should be true if cache is good)
                        let foundInGap = false;
                        for (const [startStr, endStr] of this.gaps) {
                            const startNum = parseFloat(startStr);
                            const endNum = endStr === Infinity ? Infinity : parseFloat(endStr);
                            if (preferredNum >= startNum && preferredNum < endNum) {
                                foundInGap = true;
                                break;
                            }
                        }
                        if (foundInGap) {
                            console.log(`FrequencyGapTracker: Preferred frequency ${normalizedPreferredStr} is available.`);
                            return preferredNum; // Return the number
                        } else {
                            console.warn(`FrequencyGapTracker: Preferred frequency ${normalizedPreferredStr} not in taken set, but not found within any gap. Cache might be stale. Proceeding with search.`);
                            // Fall through to search upwards
                        }
                    } else {
                        console.log(`FrequencyGapTracker: Preferred frequency ${normalizedPreferredStr} is already taken. Searching upwards.`);
                        // Start searching from the *next* possible frequency
                        searchStartNum = parseFloat((preferredNum + this.step).toFixed(1));
                    }
                } else {
                    console.log(`FrequencyGapTracker: Preferred frequency ${preferredStartFrequencyStr} (parsed ${preferredNum}) is below minimum ${this.minFreqStr}. Searching from minimum.`);
                    // searchStartNum remains this.minFreq
                }
            } catch (e) {
                console.log(`FrequencyGapTracker: Invalid preferred frequency format (${preferredStartFrequencyStr}). Searching from minimum ${this.minFreqStr}.`);
                // searchStartNum remains this.minFreq
            }
        } else {
            console.log(`FrequencyGapTracker: No preferred frequency. Searching from minimum ${this.minFreqStr}.`);
        }

        // 2. Search through sorted gaps for the first available frequency >= searchStartNum
        console.log(`FrequencyGapTracker: Searching for the first available frequency >= ${searchStartNum.toFixed(1)}`);

        // Sort gap starts numerically
        const sortedGapStartsNum = [...this.gaps.keys()]
            .map(f => parseFloat(f)) // Convert string keys to numbers
            .sort((a, b) => a - b);

        for (const gapStartNum of sortedGapStartsNum) {
            const gapStartStr = gapStartNum.toFixed(1); // Get corresponding string key
            const gapEnd = this.gaps.get(gapStartStr); // Get end using string key ("YYY.Y" or Infinity)
            const gapEndNum = gapEnd === Infinity ? Infinity : parseFloat(gapEnd);

            // Scenario A: Gap starts at or after our search point
            if (gapStartNum >= searchStartNum) {
                // The start of this gap is the first candidate, check if it's actually free
                if (!this.takenFrequencies.has(gapStartStr)) {
                    console.log(`FrequencyGapTracker: Found lowest available >= ${searchStartNum.toFixed(1)}: ${gapStartStr} (in gap ${gapStartStr}-${gapEnd})`);
                    return gapStartNum; // Return number
                } else {
                    // If gap start is taken (cache inconsistency?), find the next free spot *within* this gap
                    let nextInGapNum = parseFloat((gapStartNum + this.step).toFixed(1));
                    while (nextInGapNum < gapEndNum) {
                        const nextInGapStr = nextInGapNum.toFixed(1);
                        if (!this.takenFrequencies.has(nextInGapStr)) {
                            console.log(`FrequencyGapTracker: Found next available within gap A: ${nextInGapStr}`);
                            return nextInGapNum; // Return number
                        }
                        nextInGapNum = parseFloat((nextInGapNum + this.step).toFixed(1));
                    }
                    // If the whole gap segment was taken, continue to the next gap
                }
            }
            // Scenario B: Gap starts *before* search point, but *overlaps* it (ends after search point)
            else if (gapEndNum > searchStartNum) {
                // The first potential candidate is searchStartNum itself, if it's free
                const searchStartStr = searchStartNum.toFixed(1);
                if (!this.takenFrequencies.has(searchStartStr)) {
                    console.log(`FrequencyGapTracker: Found available frequency at search start: ${searchStartStr} (in gap ${gapStartStr}-${gapEnd})`);
                    return searchStartNum; // Return number
                } else {
                    // If searchStartNum is taken, find the next free spot within the overlapping part of the gap
                    let nextInGapNum = parseFloat((searchStartNum + this.step).toFixed(1));
                    while (nextInGapNum < gapEndNum) {
                        const nextInGapStr = nextInGapNum.toFixed(1);
                        if (!this.takenFrequencies.has(nextInGapStr)) {
                            console.log(`FrequencyGapTracker: Found next available within overlapping gap B: ${nextInGapStr}`);
                            return nextInGapNum; // Return number
                        }
                        nextInGapNum = parseFloat((nextInGapNum + this.step).toFixed(1));
                    }
                    // If the rest of the overlapping gap is taken, continue to the next gap
                }
            }
        }

        console.log(`FrequencyGapTracker: No available frequency found >= ${searchStartNum.toFixed(1)}.`);
        return null; // No suitable frequency found
    }

    /**
     * Checks whether the given frequency string is available based on current cache.
     * @param {string|number} frequencyInput - Frequency input to check.
     * @returns {boolean} - True if the frequency is considered available, false otherwise.
     */
    isFrequencyAvailable(frequencyInput) {
        if (!this.initialized) {
            console.error('FrequencyGapTracker: isFrequencyAvailable called before initialization.');
            return false; // Cannot determine availability
            // throw new Error('FrequencyGapTracker not initialized');
        }

        let normalizedFreqStr;
        let normalizedFreqNum;
        try {
            // Validate and normalize input
            if (!isValidFrequency(frequencyInput)) return false;
            normalizedFreqStr = normalizeFrequency(frequencyInput);
            normalizedFreqNum = parseFloat(normalizedFreqStr);
        } catch (e) {
            console.error(`isFrequencyAvailable: Invalid frequency format: ${frequencyInput}`);
            return false;
        }

        // 1. Check if explicitly taken
        if (this.takenFrequencies.has(normalizedFreqStr)) {
            // console.log(`isFrequencyAvailable: Frequency ${normalizedFreqStr} is in taken set.`);
            return false;
        }

        // 2. Check if it falls within any known gap [start, end)
        for (const [startStr, endStr] of this.gaps) {
            const startNum = parseFloat(startStr);
            const endNum = endStr === Infinity ? Infinity : parseFloat(endStr);
            if (normalizedFreqNum >= startNum && normalizedFreqNum < endNum) {
                // console.log(`isFrequencyAvailable: Frequency ${normalizedFreqStr} is available (in gap ${startStr}-${endStr})`);
                return true; // Found in a gap, therefore available
            }
        }

        // 3. If not taken and not in a gap, cache might be stale or it's outside tracked range (e.g., < 130.0)
        // console.log(`isFrequencyAvailable: Frequency ${normalizedFreqStr} is not in taken set but not in any available gap. Reporting as unavailable.`);
        return false; // Report as unavailable based on current cache state
    }

    /**
     * Adds the frequency string to taken set and triggers gap update (async).
     * @param {string|number} frequencyInput - Frequency input to add.
     * @returns {Promise<void>} - Resolves quickly, cache update runs in background.
     */
    async addTakenFrequency(frequencyInput) {
        let normalizedFreq;
        try {
            if (!isValidFrequency(frequencyInput)) throw new Error("Invalid format");
            normalizedFreq = normalizeFrequency(frequencyInput); // Validate and format
        } catch (e) {
            console.error(`addTakenFrequency: Invalid frequency format: ${frequencyInput}`);
            return; // Don't add invalid frequency
        }

        if (!this.takenFrequencies.has(normalizedFreq)) {
            this.takenFrequencies.add(normalizedFreq);
            console.log(`FrequencyGapTracker: Added frequency ${normalizedFreq} to taken set. Triggering async gap update.`);
            // Update gaps asynchronously without waiting (fire and forget)
            this.updateGapsCache().catch(err => console.error("FrequencyGapTracker: Error during async gap update after add:", err));
        } else {
            // console.log(`FrequencyGapTracker: Frequency ${normalizedFreq} was already in taken set.`);
        }
    }

    /**
     * Removes the frequency string from taken set and triggers gap update (async).
     * @param {string|number} frequencyInput - Frequency input to remove.
     * @returns {Promise<void>} - Resolves quickly, cache update runs in background.
     */
    async removeTakenFrequency(frequencyInput) {
        let normalizedFreq;
        try {
            if (!isValidFrequency(frequencyInput)) throw new Error("Invalid format");
            normalizedFreq = normalizeFrequency(frequencyInput); // Validate and format
        } catch (e) {
            console.error(`removeTakenFrequency: Invalid frequency format: ${frequencyInput}`);
            return; // Don't remove invalid frequency
        }

        if (this.takenFrequencies.delete(normalizedFreq)) {
            console.log(`FrequencyGapTracker: Removed frequency ${normalizedFreq} from taken set. Triggering async gap update.`);
            // Update gaps asynchronously without waiting
            this.updateGapsCache().catch(err => console.error("FrequencyGapTracker: Error during async gap update after remove:", err));
        } else {
            // console.log(`FrequencyGapTracker: Frequency ${normalizedFreq} was not in taken set for removal.`);
        }
    }

    /**
     * Verifies cache against DB (optional utility, potentially slow).
     * @param {string|number} frequencyInput - Frequency to verify.
     */
    async verifyWithDb(frequencyInput) {
        if (!this.initialized) return; // Don't verify if not ready
        let normalizedFreq;
        try {
            if (!isValidFrequency(frequencyInput)) return;
            normalizedFreq = normalizeFrequency(frequencyInput);
        } catch { return; } // Ignore invalid format

        console.log(`FrequencyGapTracker: Verifying ${normalizedFreq} against DB...`);
        try {
            const dbWavelength = await WavelengthModel.findByFrequency(normalizedFreq);
            const existsInCache = this.takenFrequencies.has(normalizedFreq);

            if (dbWavelength && !existsInCache) {
                console.warn(`DB Verification: Frequency ${normalizedFreq} exists in DB but wasn't in tracker! Adding and updating cache.`);
                // Use addTakenFrequency which triggers async update
                await this.addTakenFrequency(normalizedFreq);
            } else if (!dbWavelength && existsInCache) {
                console.warn(`DB Verification: Frequency ${normalizedFreq} is in tracker but not in DB! Removing and updating cache.`);
                // Use removeTakenFrequency which triggers async update
                await this.removeTakenFrequency(normalizedFreq);
            } else {
                console.log(`DB Verification: Cache for ${normalizedFreq} matches DB state (Exists: ${!!dbWavelength}).`);
            }
        } catch (error) {
            console.error(`FrequencyGapTracker: Error verifying frequency ${normalizedFreq} with DB:`, error);
        }
    }
}

// Singleton instance
const frequencyTracker = new FrequencyGapTracker();
module.exports = frequencyTracker; // Export instance