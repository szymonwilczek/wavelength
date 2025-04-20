const WavelengthModel = require('../models/wavelength');
const { normalizeFrequency } = require('../utils/helpers');

/**
 * Class responsible for tracking gaps in frequencies
 */
class FrequencyGapTracker {
    constructor() {
        this.gaps = new Map(); // Przechowuje [startGap, endGap)
        this.initialized = false;
        this.takenFrequencies = new Set(); // Przechowuje zajęte częstotliwości
    }

    /**
     * Initializes the FrequencyGapTracker by updating the gaps cache
     * @returns {Promise<boolean>} - True if initialization was successful, false otherwise
     */
    async initialize() {
        try {
            await this.updateGapsCache();
            this.initialized = true;
            console.log('FrequencyGapTracker initialized.');
            // console.log('Initial gaps:', [...this.gaps.entries()]); // Może być bardzo dużo
            // console.log('Initial taken frequencies:', [...this.takenFrequencies]); // Może być bardzo dużo
            return true;
        } catch (error) {
            console.error('Error initializing frequency tracker:', error);
            return false;
        }
    }

    /**
     * Updates the gaps cache by querying the database for existing wavelengths
     * @returns {Promise<boolean>} - True if the cache was updated successfully, false otherwise
     */
    async updateGapsCache() {
        try {
            const wavelengths = await WavelengthModel.findAll();
            // console.log(`Updating gaps cache: Found ${wavelengths.length} registered wavelengths in database`);

            const frequencies = wavelengths.map(w => normalizeFrequency(w.frequency));
            this.takenFrequencies = new Set(frequencies);

            const sortedFreqs = [...this.takenFrequencies].sort((a, b) => a - b);
            // console.log('Sorted taken frequencies:', sortedFreqs);

            this.gaps.clear();
            const minFreq = 130.0;
            const step = 0.1;
            let lastTaken = minFreq - step; // Start before the minimum possible frequency

            // Check gap before the first taken frequency
            if (sortedFreqs.length === 0 || sortedFreqs[0] > minFreq) {
                const endGap = sortedFreqs.length > 0 ? sortedFreqs[0] : Infinity;
                this.gaps.set(minFreq, endGap);
                // console.log(`Found gap at beginning: ${minFreq} - ${endGap}`);
                lastTaken = minFreq - step; // Reset lastTaken as we start from minFreq
            }


            // Find gaps between taken frequencies
            for (const currentTaken of sortedFreqs) {
                // Skip if currentTaken is the absolute minimum (already handled)
                if (currentTaken <= minFreq && this.gaps.has(minFreq)) {
                    lastTaken = currentTaken;
                    continue;
                }

                const potentialGapStart = normalizeFrequency(lastTaken + step);
                if (currentTaken > potentialGapStart) {
                    this.gaps.set(potentialGapStart, currentTaken);
                    // console.log(`Found gap: ${potentialGapStart} - ${currentTaken}`);
                }
                lastTaken = currentTaken;
            }

            // Add gap after the last taken frequency
            const gapAfterLastStart = normalizeFrequency(lastTaken + step);
            this.gaps.set(gapAfterLastStart, Infinity);
            // console.log(`Found gap after last: ${gapAfterLastStart} - Infinity`);

            // console.log('Gaps cache updated:', [...this.gaps.entries()]);
            return true;
        } catch (error) {
            console.error('Error updating frequency gaps cache:', error);
            console.error(error.stack);
            return false;
        }
    }

    /**
     * Finds the lowest available frequency, optionally starting from a preferred frequency.
     * @param {number|null} [preferredStartFrequency=null] - The preferred frequency to start searching from (must be >= 130.0).
     * @returns {number|null} - Lowest available frequency matching criteria or null if not found.
     */
    findLowestAvailableFrequency(preferredStartFrequency = null) {
        if (!this.initialized) {
            console.error('FrequencyGapTracker not initialized');
            throw new Error('FrequencyGapTracker not initialized');
        }

        const minAllowedFreq = 130.0;
        let searchStartFreq = minAllowedFreq;

        // Validate and normalize preferred frequency
        if (preferredStartFrequency !== null && typeof preferredStartFrequency === 'number' && preferredStartFrequency >= minAllowedFreq) {
            const normalizedPreferred = normalizeFrequency(preferredStartFrequency);
            console.log(`Preferred frequency provided: ${preferredStartFrequency}, normalized: ${normalizedPreferred}`);

            // 1. Check if the exact preferred frequency is available
            if (!this.takenFrequencies.has(normalizedPreferred)) {
                // Verify it falls within a known gap (sanity check)
                for (const [start, end] of this.gaps) {
                    if (normalizedPreferred >= start && normalizedPreferred < end) {
                        console.log(`Preferred frequency ${normalizedPreferred} Hz is available.`);
                        // Optionally verify with DB here if needed, but tracker should be reliable
                        // await this.verifyWithWavelengthService(normalizedPreferred);
                        return normalizedPreferred;
                    }
                }
                console.warn(`Preferred frequency ${normalizedPreferred} Hz not in taken set, but not found within any gap. Rebuilding cache might be needed.`);
                // Fall through to search upwards if it wasn't in a gap for some reason
            } else {
                console.log(`Preferred frequency ${normalizedPreferred} Hz is already taken.`);
            }
            // If preferred is taken or invalid, start searching from it upwards
            searchStartFreq = normalizedPreferred;
        } else if (preferredStartFrequency !== null) {
            console.log(`Invalid preferred frequency (${preferredStartFrequency}). Searching from default minimum ${minAllowedFreq} Hz.`);
        } else {
            console.log(`No preferred frequency. Searching from default minimum ${minAllowedFreq} Hz.`);
        }


        // 2. Search for the first available frequency >= searchStartFreq
        console.log(`Searching for the first available frequency >= ${searchStartFreq} Hz`);

        // Sort gap starts to ensure we find the lowest first
        const sortedGapStarts = [...this.gaps.keys()].sort((a, b) => a - b);

        for (const gapStart of sortedGapStarts) {
            const gapEnd = this.gaps.get(gapStart);

            // Find the first gap that starts at or after our search start point
            if (gapStart >= searchStartFreq) {
                // Check if this start is actually taken (shouldn't happen if cache is correct, but safety check)
                if (!this.takenFrequencies.has(gapStart)) {
                    console.log(`Found lowest available frequency >= ${searchStartFreq}: ${gapStart} Hz (within gap ${gapStart}-${gapEnd})`);
                    // Optionally verify with DB
                    // await this.verifyWithWavelengthService(gapStart);
                    return gapStart;
                } else {
                    console.warn(`Gap start ${gapStart} is marked as taken. Searching for next possible value.`);
                    // If the gap start itself is taken, find the next possible value within this gap
                    let nextInGap = normalizeFrequency(gapStart + 0.1);
                    while (nextInGap < gapEnd && this.takenFrequencies.has(nextInGap)) {
                        nextInGap = normalizeFrequency(nextInGap + 0.1);
                    }
                    if (nextInGap < gapEnd) {
                        console.log(`Found next available frequency within the gap: ${nextInGap} Hz`);
                        return nextInGap;
                    }
                    // If the whole gap segment was somehow taken, continue to the next gap
                }
            }
            // If the gap starts *before* our search frequency, but *overlaps* it
            else if (gapEnd > searchStartFreq) {
                // The first available frequency in this gap *at or after* searchStartFreq is searchStartFreq itself,
                // but only if it's not taken.
                if (!this.takenFrequencies.has(searchStartFreq)) {
                    console.log(`Found available frequency starting from preferred: ${searchStartFreq} Hz (within gap ${gapStart}-${gapEnd})`);
                    return searchStartFreq;
                } else {
                    // If searchStartFreq is taken, find the next available within this gap
                    let nextInGap = normalizeFrequency(searchStartFreq + 0.1);
                    while (nextInGap < gapEnd && this.takenFrequencies.has(nextInGap)) {
                        nextInGap = normalizeFrequency(nextInGap + 0.1);
                    }
                    if (nextInGap < gapEnd) {
                        console.log(`Found next available frequency within the overlapping gap: ${nextInGap} Hz`);
                        return nextInGap;
                    }
                    // If the rest of the overlapping gap is taken, continue to the next gap
                }
            }
        }

        console.log(`No available frequency found >= ${searchStartFreq} Hz.`);
        return null; // No suitable frequency found
    }


    /**
     * Additional verification using WavelengthModel (optional, for debugging/robustness)
     * @param {number} frequency - Frequency to be verified
     */
    async verifyWithWavelengthService(frequency) {
        try {
            const normalizedFreq = normalizeFrequency(frequency);
            const exists = await WavelengthModel.findByFrequency(normalizedFreq);

            if (exists && !this.takenFrequencies.has(normalizedFreq)) {
                console.warn(`DB Verification: Frequency ${normalizedFreq} exists in DB but wasn't in taken set! Adding and updating cache.`);
                this.takenFrequencies.add(normalizedFreq);
                await this.updateGapsCache(); // Rebuild gaps based on new info
            } else if (!exists && this.takenFrequencies.has(normalizedFreq)) {
                console.warn(`DB Verification: Frequency ${normalizedFreq} is in taken set but not in DB! Removing and updating cache.`);
                this.takenFrequencies.delete(normalizedFreq);
                await this.updateGapsCache();
            }

        } catch (error) {
            console.error(`Error verifying frequency ${frequency} with DB:`, error);
        }
    }

    /**
     * Checks whether the given frequency is available (not in taken set and within a gap)
     * @param {number} frequency - Frequency to be checked
     * @returns {boolean} - True if the frequency is available, false otherwise
     */
    isFrequencyAvailable(frequency) {
        if (!this.initialized) {
            throw new Error('FrequencyGapTracker not initialized');
        }

        const normalizedFreq = normalizeFrequency(frequency);

        if (this.takenFrequencies.has(normalizedFreq)) {
            // console.log(`isFrequencyAvailable: Frequency ${normalizedFreq} is in taken set.`);
            return false;
        }

        // Check if it falls within any known gap [start, end)
        for (const [start, end] of this.gaps) {
            if (normalizedFreq >= start && normalizedFreq < end) {
                // console.log(`isFrequencyAvailable: Frequency ${normalizedFreq} is available (in gap ${start}-${end})`);
                return true;
            }
        }

        // console.log(`isFrequencyAvailable: Frequency ${normalizedFreq} is not in taken set but not in any available gap.`);
        // This might indicate a need to update the cache
        return false;
    }

    /**
     * Adds the frequency to taken frequencies and updates the gaps
     * @param {number} frequency - Frequency to be added
     */
    async addTakenFrequency(frequency) {
        const normalizedFreq = normalizeFrequency(frequency);
        if (!this.takenFrequencies.has(normalizedFreq)) {
            this.takenFrequencies.add(normalizedFreq);
            await this.updateGapsCache(); // Update gaps based on the new taken frequency
            console.log(`Added frequency ${normalizedFreq} to taken set and updated gaps.`);
        } else {
            console.log(`Frequency ${normalizedFreq} was already in taken set.`);
        }
    }

    /**
     * Removes the frequency from taken frequencies and updates the gaps
     * @param {number} frequency - Frequency to be removed
     */
    async removeTakenFrequency(frequency) {
        const normalizedFreq = normalizeFrequency(frequency);
        if (this.takenFrequencies.delete(normalizedFreq)) {
            await this.updateGapsCache(); // Update gaps now that frequency is free
            console.log(`Removed frequency ${normalizedFreq} from taken set and updated gaps.`);
        } else {
            console.log(`Frequency ${normalizedFreq} was not in taken set.`);
        }
    }
}

// Singleton instance
const frequencyTracker = new FrequencyGapTracker();
module.exports = frequencyTracker;