// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\routes\api.js
/**
 * API router for Wavelength application
 */
const express = require("express");
const wavelengthService = require("../services/wavelengthService"); // Uses string frequencies
// connectionManager not needed directly if service handles lookups
const { normalizeFrequency, isValidFrequency } = require("../utils/helpers"); // Use string helpers
// frequencyTracker not needed directly if service handles finding next

const router = express.Router();

/**
 * Endpoint checking server status
 * @route GET /health
 */
router.get("/health", (req, res) => {
  res.status(200).json({ status: "ok", timestamp: new Date().toISOString() });
});

/**
 * Returns the next available frequency as a string "XXX.X"
 * @route GET /api/next-available-frequency
 * @queryparam {string} [preferredStartFrequency] - Optional preferred frequency (e.g., "145.5") >= 130.0
 */
router.get("/api/next-available-frequency", async (req, res) => {
  console.log("API: Request received for /api/next-available-frequency");
  try {
    let preferredStartFrequencyInput = req.query.preferredStartFrequency || null;
    console.log(`API: Raw preferredStartFrequency query param: ${preferredStartFrequencyInput}`);

    // Let the service layer handle validation and finding the frequency
    const nextFrequencyStr = await wavelengthService.findNextAvailableFrequency(preferredStartFrequencyInput);

    if (nextFrequencyStr !== null) {
      console.log(`API: Found next available frequency: ${nextFrequencyStr}`);
      res.json({
        frequency: nextFrequencyStr, // Return string frequency "XXX.X"
        status: "success",
      });
    } else {
      console.log("API: No available frequencies found matching the criteria.");
      res.status(404).json({
        error: "No available frequencies found matching the criteria",
        status: "error",
      });
    }
  } catch (err) {
    // Catch errors from the service/tracker (e.g., tracker not initialized)
    console.error("API: Error finding next available frequency:", err);
    res.status(500).json({
      error: err.message || "Server error finding next frequency", // Provide specific error if available
      status: "error",
    });
  }
});


/**
 * Returns a list of all wavelengths currently in the database (frequency as string)
 * @route GET /api/wavelengths
 */
router.get("/api/wavelengths", async (req, res) => {
  console.log("API: Request received for /api/wavelengths");
  try {
    // Service returns objects with string frequency and mapped fields
    const wavelengths = await wavelengthService.getAllWavelengths();

    // Format response (already has string frequency and correct field names)
    const response = wavelengths.map((w) => ({
      frequency: w.frequency, // String "XXX.X"
      name: w.name,
      isPasswordProtected: w.isPasswordProtected,
      createdAt: w.createdAt,
      // Do not expose passwordHash or hostSocketId via public API
    }));

    console.log(`API: Returning ${response.length} wavelengths.`);
    res.json({
      wavelengths: response,
      count: response.length,
      status: "success",
    });
  } catch (err) {
    console.error("API: Error getting wavelengths:", err);
    res.status(500).json({
      error: "Server error getting wavelengths",
      status: "error",
    });
  }
});

/**
 * Returns information about a specific frequency (string "XXX.X")
 * Checks memory (online status) then database.
 * @route GET /api/wavelengths/:frequency
 */
router.get("/api/wavelengths/:frequency", async (req, res) => {
  const frequencyInput = req.params.frequency;
  console.log(`API: Request received for /api/wavelengths/${frequencyInput}`);

  let normalizedFrequencyStr;
  try {
    // Validate/Normalize the frequency from the path parameter
    if (!isValidFrequency(frequencyInput)) {
      throw new Error("Invalid frequency format. Must be a positive number.");
    }
    normalizedFrequencyStr = normalizeFrequency(frequencyInput);
  } catch (e) {
    console.warn(`API: Invalid frequency format in path: ${frequencyInput}`);
    res.status(400).json({ error: e.message || "Invalid frequency format", status: "error" });
    return;
  }

  try {
    // Use the service layer which checks memory then DB
    const wavelengthInfo = await wavelengthService.getWavelength(normalizedFrequencyStr);

    if (wavelengthInfo) {
      console.log(`API: Found info for ${normalizedFrequencyStr}. Online: ${wavelengthInfo.isOnline}`);
      // wavelengthInfo contains frequency (string), name, isPasswordProtected, isOnline, createdAt
      res.json({
        frequency: wavelengthInfo.frequency, // String "XXX.X"
        name: wavelengthInfo.name,
        isPasswordProtected: wavelengthInfo.isPasswordProtected,
        isOnline: wavelengthInfo.isOnline, // Indicates if active in memory (host connected)
        createdAt: wavelengthInfo.createdAt, // Available if found in DB
        // Do not expose passwordHash or hostSocketId
        status: "success",
      });
    } else {
      // Not found in memory or DB
      console.log(`API: Wavelength ${normalizedFrequencyStr} not found.`);
      res.status(404).json({
        error: "Wavelength not found",
        status: "error",
      });
    }
  } catch (err) {
    console.error(`API: Error getting wavelength information for ${normalizedFrequencyStr}:`, err);
    res.status(500).json({
      error: "Server error getting wavelength information",
      status: "error",
    });
  }
});

module.exports = router;