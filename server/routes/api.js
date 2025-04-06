/**
 * API router for Wavelength application
 */
const express = require("express");
const wavelengthService = require("../services/wavelengthService");
const connectionManager = require("../websocket/connectionManager");
const { normalizeFrequency } = require("../utils/helpers");

const router = express.Router();

/**
 * Endpoint checking server status
 * @route GET /health
 */
router.get("/health", (req, res) => {
  res.status(200).json({ status: "ok" });
});

/**
 * Returns the next available frequency
 * @route GET /api/next-available-frequency
 */
router.get("/api/next-available-frequency", async (req, res) => {
  try {
    const startFrequency = connectionManager.lastRegisteredFrequency;
    console.log(
      `Looking for next available frequency starting from ${startFrequency} Hz`
    );

    const nextFrequency = await wavelengthService.findNextAvailableFrequency(
      startFrequency
    );

    if (nextFrequency) {
      console.log(`Found next available frequency: ${nextFrequency} Hz`);
      res.json({
        frequency: nextFrequency,
        status: "success",
      });
    } else {
      console.log("No available frequencies found");
      res.status(404).json({
        error: "No available frequencies found",
        status: "error",
      });
    }
  } catch (err) {
    console.error("Error finding next available frequency:", err);
    res.status(500).json({
      error: "Server error",
      status: "error",
    });
  }
});

/**
 * Returns a list of all active wavelengths
 * @route GET /api/wavelengths
 */
router.get("/api/wavelengths", async (req, res) => {
  try {
    const wavelengths = await wavelengthService.getAllWavelengths();

    const response = wavelengths.map((w) => ({
      frequency: parseFloat(w.frequency),
      name: w.name,
      isPasswordProtected: w.isPasswordProtected,
      createdAt: w.createdAt,
    }));

    res.json({
      wavelengths: response,
      count: response.length,
      status: "success",
    });
  } catch (err) {
    console.error("Error getting wavelengths:", err);
    res.status(500).json({
      error: "Server error",
      status: "error",
    });
  }
});

/**
 * Returns information at a specific frequency
 * @route GET /api/wavelengths/:frequency
 */
router.get("/api/wavelengths/:frequency", async (req, res) => {
  try {
    const frequency = normalizeFrequency(req.params.frequency);

    // At the beginning, check if the frequency is in the memory of active wavelengths
    const memoryWavelength = connectionManager.activeWavelengths.get(frequency);
    if (memoryWavelength) {
      res.json({
        frequency,
        name: memoryWavelength.name,
        isPasswordProtected: memoryWavelength.isPasswordProtected,
        isOnline: true,
        status: "success",
      });
      return;
    }

    // Only if it is not in memory, we try to get it from the database
    const dbWavelength = await wavelengthService.getWavelength(frequency);
    if (dbWavelength) {
      res.json({
        frequency: parseFloat(dbWavelength.frequency),
        name: dbWavelength.name,
        isPasswordProtected: dbWavelength.is_password_protected,
        isOnline: false,
        status: "success",
      });
      return;
    }

    res.status(404).json({
      error: "Wavelength not found",
      status: "error",
    });
  } catch (err) {
    console.error(`Error getting wavelength information:`, err);
    res.status(500).json({
      error: "Server error",
      status: "error",
    });
  }
});

module.exports = router;
