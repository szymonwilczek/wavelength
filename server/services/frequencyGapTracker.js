const WavelengthModel = require('../models/wavelength'); 
const { normalizeFrequency, isValidFrequency } = require('../utils/helpers'); 

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
            return await this._updatePromise; 
        }

        console.log('FrequencyGapTracker: Initializing...');
        this._updatePromise = this._initializeInternal();
        try {
            return await this._updatePromise;
        } finally {
            this._updatePromise = null;
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
            this.initialized = false; 
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
            this._updatePromise = null;
        }
    }

    async _updateGapsCacheInternal() {
        try {
            const wavelengths = await WavelengthModel.findAll();
            console.log(`FrequencyGapTracker: Found ${wavelengths.length} wavelengths in DB for cache update.`);

            // Normalize just in case, though model should return normalized strings
            const frequencies = wavelengths.map(w => normalizeFrequency(w.frequency));
            this.takenFrequencies = new Set(frequencies);

            const sortedFreqsNum = [...this.takenFrequencies]
                .map(f => parseFloat(f))
                .sort((a, b) => a - b);

            this.gaps.clear();
            let lastTakenNum = this.minFreq - this.step;

            if (sortedFreqsNum.length === 0 || sortedFreqsNum[0] > this.minFreq) {
                const endGapNum = sortedFreqsNum.length > 0 ? sortedFreqsNum[0] : Infinity;
                const endGapStr = endGapNum === Infinity ? Infinity : endGapNum.toFixed(1);
                this.gaps.set(this.minFreqStr, endGapStr); 
            }

            for (const currentTakenNum of sortedFreqsNum) {
                if (currentTakenNum <= this.minFreq && this.gaps.has(this.minFreqStr)) {
                    lastTakenNum = currentTakenNum;
                    continue;
                }

                const potentialGapStartNum = parseFloat((lastTakenNum + this.step).toFixed(1));

                if (currentTakenNum > potentialGapStartNum) {
                    const startGapStr = potentialGapStartNum.toFixed(1);
                    const endGapStr = currentTakenNum.toFixed(1);
                    this.gaps.set(startGapStr, endGapStr); 
                }
                lastTakenNum = currentTakenNum;
            }

            const gapAfterLastStartNum = parseFloat((lastTakenNum + this.step).toFixed(1));
            const gapAfterLastStartStr = gapAfterLastStartNum.toFixed(1);
            this.gaps.set(gapAfterLastStartStr, Infinity); 
            
            console.log(`FrequencyGapTracker: Gaps cache updated. ${this.gaps.size} gaps found.`);
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
            return null;
        }

        let searchStartNum = this.minFreq; 

        if (preferredStartFrequencyStr !== null) {
            try {
                const normalizedPreferredStr = normalizeFrequency(preferredStartFrequencyStr);
                const preferredNum = parseFloat(normalizedPreferredStr);

                if (preferredNum >= this.minFreq) {
                    console.log(`FrequencyGapTracker: Preferred frequency provided: ${normalizedPreferredStr}`);
                    searchStartNum = preferredNum; 

                    if (!this.takenFrequencies.has(normalizedPreferredStr)) {
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
                            return preferredNum; 
                        } else {
                            console.warn(`FrequencyGapTracker: Preferred frequency ${normalizedPreferredStr} not in taken set, but not found within any gap. Cache might be stale. Proceeding with search.`);
                        }
                    } else {
                        console.log(`FrequencyGapTracker: Preferred frequency ${normalizedPreferredStr} is already taken. Searching upwards.`);
                        searchStartNum = parseFloat((preferredNum + this.step).toFixed(1));
                    }
                } else {
                    console.log(`FrequencyGapTracker: Preferred frequency ${preferredStartFrequencyStr} (parsed ${preferredNum}) is below minimum ${this.minFreqStr}. Searching from minimum.`);
                }
            } catch (e) {
                console.log(`FrequencyGapTracker: Invalid preferred frequency format (${preferredStartFrequencyStr}). Searching from minimum ${this.minFreqStr}.`);
            }
        } else {
            console.log(`FrequencyGapTracker: No preferred frequency. Searching from minimum ${this.minFreqStr}.`);
        }

        console.log(`FrequencyGapTracker: Searching for the first available frequency >= ${searchStartNum.toFixed(1)}`);

        const sortedGapStartsNum = [...this.gaps.keys()]
            .map(f => parseFloat(f)) 
            .sort((a, b) => a - b);

        for (const gapStartNum of sortedGapStartsNum) {
            const gapStartStr = gapStartNum.toFixed(1); 
            const gapEnd = this.gaps.get(gapStartStr); 
            const gapEndNum = gapEnd === Infinity ? Infinity : parseFloat(gapEnd);

            if (gapStartNum >= searchStartNum) {
                if (!this.takenFrequencies.has(gapStartStr)) {
                    console.log(`FrequencyGapTracker: Found lowest available >= ${searchStartNum.toFixed(1)}: ${gapStartStr} (in gap ${gapStartStr}-${gapEnd})`);
                    return gapStartNum;
                } else {
                    let nextInGapNum = parseFloat((gapStartNum + this.step).toFixed(1));
                    while (nextInGapNum < gapEndNum) {
                        const nextInGapStr = nextInGapNum.toFixed(1);
                        if (!this.takenFrequencies.has(nextInGapStr)) {
                            console.log(`FrequencyGapTracker: Found next available within gap A: ${nextInGapStr}`);
                            return nextInGapNum;
                        }
                        nextInGapNum = parseFloat((nextInGapNum + this.step).toFixed(1));
                    }
                }
            }
            else if (gapEndNum > searchStartNum) {
                const searchStartStr = searchStartNum.toFixed(1);
                if (!this.takenFrequencies.has(searchStartStr)) {
                    console.log(`FrequencyGapTracker: Found available frequency at search start: ${searchStartStr} (in gap ${gapStartStr}-${gapEnd})`);
                    return searchStartNum; 
                } else {
                    while (nextInGapNum < gapEndNum) {
                        const nextInGapStr = nextInGapNum.toFixed(1);
                        if (!this.takenFrequencies.has(nextInGapStr)) {
                            console.log(`FrequencyGapTracker: Found next available within overlapping gap B: ${nextInGapStr}`);
                            return nextInGapNum;
                        }
                        nextInGapNum = parseFloat((nextInGapNum + this.step).toFixed(1));
                    }
                }
            }
        }

        console.log(`FrequencyGapTracker: No available frequency found >= ${searchStartNum.toFixed(1)}.`);
        return null; 
    }

    /**
     * Checks whether the given frequency string is available based on current cache.
     * @param {string|number} frequencyInput - Frequency input to check.
     * @returns {boolean} - True if the frequency is considered available, false otherwise.
     */
    isFrequencyAvailable(frequencyInput) {
        if (!this.initialized) {
            console.error('FrequencyGapTracker: isFrequencyAvailable called before initialization.');
            return false;
        }

        let normalizedFreqStr;
        let normalizedFreqNum;
        try {
            if (!isValidFrequency(frequencyInput)) return false;
            normalizedFreqStr = normalizeFrequency(frequencyInput);
            normalizedFreqNum = parseFloat(normalizedFreqStr);
        } catch (e) {
            console.error(`isFrequencyAvailable: Invalid frequency format: ${frequencyInput}`);
            return false;
        }

        if (this.takenFrequencies.has(normalizedFreqStr)) {
            return false;
        }

        for (const [startStr, endStr] of this.gaps) {
            const startNum = parseFloat(startStr);
            const endNum = endStr === Infinity ? Infinity : parseFloat(endStr);
            if (normalizedFreqNum >= startNum && normalizedFreqNum < endNum) {
                return true; 
            }
        }
        return false; 
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
            normalizedFreq = normalizeFrequency(frequencyInput);
        } catch (e) {
            console.error(`addTakenFrequency: Invalid frequency format: ${frequencyInput}`);
            return; 
        }

        if (!this.takenFrequencies.has(normalizedFreq)) {
            this.takenFrequencies.add(normalizedFreq);
            console.log(`FrequencyGapTracker: Added frequency ${normalizedFreq} to taken set. Triggering async gap update.`);
            this.updateGapsCache().catch(err => console.error("FrequencyGapTracker: Error during async gap update after add:", err));
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
            normalizedFreq = normalizeFrequency(frequencyInput); 
        } catch (e) {
            console.error(`removeTakenFrequency: Invalid frequency format: ${frequencyInput}`);
            return;
        }

        if (this.takenFrequencies.delete(normalizedFreq)) {
            console.log(`FrequencyGapTracker: Removed frequency ${normalizedFreq} from taken set. Triggering async gap update.`);
            this.updateGapsCache().catch(err => console.error("FrequencyGapTracker: Error during async gap update after remove:", err));
        }
    }

    /**
     * Verifies cache against DB (optional utility, potentially slow).
     * @param {string|number} frequencyInput - Frequency to verify.
     */
    async verifyWithDb(frequencyInput) {
        if (!this.initialized) return; 
        let normalizedFreq;
        try {
            if (!isValidFrequency(frequencyInput)) return;
            normalizedFreq = normalizeFrequency(frequencyInput);
        } catch { return; } 

        console.log(`FrequencyGapTracker: Verifying ${normalizedFreq} against DB...`);
        try {
            const dbWavelength = await WavelengthModel.findByFrequency(normalizedFreq);
            const existsInCache = this.takenFrequencies.has(normalizedFreq);

            if (dbWavelength && !existsInCache) {
                console.warn(`DB Verification: Frequency ${normalizedFreq} exists in DB but wasn't in tracker! Adding and updating cache.`);
                await this.addTakenFrequency(normalizedFreq);
            } else if (!dbWavelength && existsInCache) {
                console.warn(`DB Verification: Frequency ${normalizedFreq} is in tracker but not in DB! Removing and updating cache.`);
                await this.removeTakenFrequency(normalizedFreq);
            } else {
                console.log(`DB Verification: Cache for ${normalizedFreq} matches DB state (Exists: ${!!dbWavelength}).`);
            }
        } catch (error) {
            console.error(`FrequencyGapTracker: Error verifying frequency ${normalizedFreq} with DB:`, error);
        }
    }
}

const frequencyTracker = new FrequencyGapTracker();
module.exports = frequencyTracker; 
