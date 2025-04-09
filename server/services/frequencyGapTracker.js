const WavelengthModel = require('../models/wavelength');
const { normalizeFrequency } = require('../utils/helpers');

/**
 * Class responsible for tracking gaps in frequencies
 */
class FrequencyGapTracker {
    constructor() {
        this.gaps = new Map();
        this.initialized = false;
        this.takenFrequencies = new Set();
    }

    /**
     *  Initializes the FrequencyGapTracker by updating the gaps cache
     * @returns {Promise<boolean>} - True if initialization was successful, false otherwise
     */
    async initialize() {
        try {
            await this.updateGapsCache();
            this.initialized = true;
            console.log('FrequencyGapTracker initialized with gaps:', [...this.gaps.entries()]);
            console.log('Taken frequencies:', [...this.takenFrequencies]);
            return true;
        } catch (error) {
            console.error('Error initializing frequency tracker:', error);
            return false;
        }
    }

    /**
     *  Updates the gaps cache by querying the database for existing wavelengths
     * @returns {Promise<boolean>} - True if the cache was updated successfully, false otherwise
     */
    async updateGapsCache() {
        try {
            const wavelengths = await WavelengthModel.findAll();
            console.log(`Found ${wavelengths.length} registered wavelengths in database`);

            const frequencies = wavelengths.map(w => normalizeFrequency(w.frequency));

            this.takenFrequencies = new Set(frequencies);

            const sortedFreqs = [...frequencies].sort((a, b) => a - b);
            console.log('Sorted frequencies:', sortedFreqs);

            this.gaps.clear();

            if (sortedFreqs.length > 0) {

                if (sortedFreqs[0] > 130.0) {
                    this.gaps.set(130.0, sortedFreqs[0]);
                    console.log(`Found gap at beginning: 130.0 - ${sortedFreqs[0]}`);
                }

                for (let i = 0; i < sortedFreqs.length - 1; i++) {
                    const current = sortedFreqs[i];
                    const next = sortedFreqs[i + 1];
                    const step = 0.1;

                    if (next - current > step) {
                        const gapStart = normalizeFrequency(current + step);
                        this.gaps.set(gapStart, next);
                        console.log(`Found gap: ${gapStart} - ${next}`);
                    }
                }

                const lastFreq = sortedFreqs[sortedFreqs.length - 1];
                const nextAfterLast = normalizeFrequency(lastFreq + 0.1);
                this.gaps.set(nextAfterLast, Infinity);
                console.log(`Found gap after last: ${nextAfterLast} - Infinity`);
            } else {
                this.gaps.set(130.0, Infinity);
                console.log('No frequencies found, setting gap: 130.0 - Infinity');
            }

            return true;
        } catch (error) {
            console.error('Error updating frequency gaps cache:', error);
            console.error(error.stack);
            return false;
        }
    }

    /**
     * Finds the lowest available frequency
     * @returns {number|null} - Lowest available frequency or null if not found
     */
    findLowestAvailableFrequency() {
        if (!this.initialized) {
            throw new Error('FrequencyGapTracker not initialized');
        }

        if (this.gaps.size === 0) {
            console.log('No gaps found in frequency spectrum');
            return null;
        }

        let lowestStart = Infinity;
        for (const [start] of this.gaps) {
            if (start < lowestStart) {
                lowestStart = start;
            }
        }

        console.log(`Lowest available frequency found: ${lowestStart}`);

        if (this.takenFrequencies.has(lowestStart)) {
            console.warn(`Warning: Frequency ${lowestStart} is marked as taken but was found in gaps`);

            let nextAvailable = lowestStart;
            while (this.takenFrequencies.has(nextAvailable)) {
                nextAvailable = normalizeFrequency(nextAvailable + 0.1);
            }
            console.log(`Adjusted to next available frequency: ${nextAvailable}`);
            return nextAvailable;
        }

        this.verifyWithWavelengthService(lowestStart);

        return lowestStart;
    }

    /**
     * Additional verification using WavelengthService
     * @param {number} frequency - Frequency to be verified
     */
    async verifyWithWavelengthService(frequency) {
        try {
            const exists = await WavelengthModel.findByFrequency(frequency);

            if (exists) {
                console.warn(`Warning: Frequency ${frequency} exists in database but wasn't detected in initial scan`);
                this.takenFrequencies.add(frequency);
                await this.updateGapsCache();
            }

        } catch (error) {
            console.error(`Error verifying frequency ${frequency}:`, error);
        }
    }

    /**
     * Checks whether the given frequency is available
     * @param {number} frequency - Frequency to be checked
     * @returns {boolean} - True if the frequency is available, false otherwise
     */
    isFrequencyAvailable(frequency) {
        if (!this.initialized) {
            throw new Error('FrequencyGapTracker not initialized');
        }

        const normalizedFreq = normalizeFrequency(frequency);

        if (this.takenFrequencies.has(normalizedFreq)) {
            console.log(`Frequency ${normalizedFreq} is already taken`);
            return false;
        }

        for (const [start, end] of this.gaps) {
            if (normalizedFreq >= start && normalizedFreq < end) {
                console.log(`Frequency ${normalizedFreq} is available (in gap ${start}-${end})`);
                return true;
            }
        }

        console.log(`Frequency ${normalizedFreq} is not in any available gap`);
        return false;
    }

    /**
     * Adds the frequency to taken frequencies and updates the gaps
     * @param {number} frequency - Frequency to be added
     */
    async addTakenFrequency(frequency) {
        const normalizedFreq = normalizeFrequency(frequency);
        this.takenFrequencies.add(normalizedFreq);
        await this.updateGapsCache();
        console.log(`Added frequency ${normalizedFreq} to taken frequencies`);
    }

    /**
     * Removes the frequency from taken frequencies and updates the gaps
     * @param {number} frequency - Frequency to be removed
     */
    async removeTakenFrequency(frequency) {
        const normalizedFreq = normalizeFrequency(frequency);
        this.takenFrequencies.delete(normalizedFreq);
        await this.updateGapsCache();
        console.log(`Removed frequency ${normalizedFreq} from taken frequencies`);
    }
}

const frequencyTracker = new FrequencyGapTracker();
module.exports = frequencyTracker;