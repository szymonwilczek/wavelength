/**
 * API router for Wavelength application
 */
const express = require("express");
const wavelengthService = require("../services/wavelengthService");
const connectionManager = require("../websocket/connectionManager");
const { normalizeFrequency } = require("../utils/helpers");
const frequencyTracker = require("../services/frequencyGapTracker");

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
    // Odczytaj opcjonalny parametr i przekonwertuj na liczbę lub null
    let preferredStartFrequency = null;
    if (req.query.preferredStartFrequency) {
      const parsedFreq = parseFloat(req.query.preferredStartFrequency);
      // Sprawdź, czy parsowanie się udało i czy wartość jest >= 130.0
      if (!isNaN(parsedFreq) && parsedFreq >= 130.0) {
        preferredStartFrequency = parsedFreq;
      } else {
        console.warn("Invalid or out-of-range preferredStartFrequency parameter received:", req.query.preferredStartFrequency);
        // Nie zwracaj błędu, po prostu zignoruj nieprawidłową wartość
      }
    }

    console.log(`Looking for next available frequency. Preferred start: ${preferredStartFrequency ?? 'None'}`);

    // Aktualizuj cache przed wyszukiwaniem (może być opcjonalne, zależy od strategii)
    // await frequencyTracker.updateGapsCache(); // Rozważ, czy to konieczne przy każdym zapytaniu

    // Przekaż preferowaną częstotliwość do trackera
    const nextFrequency = frequencyTracker.findLowestAvailableFrequency(preferredStartFrequency);

    if (nextFrequency !== null) {
      console.log(`Found next available frequency: ${nextFrequency} Hz`);

      // Dodatkowa weryfikacja (opcjonalna, tracker powinien być wiarygodny)
      // const existingInDb = await wavelengthService.getWavelength(nextFrequency);
      // const existingInMemory = connectionManager.activeWavelengths.has(nextFrequency);
      // if (existingInDb || existingInMemory) { ... }

      res.json({
        frequency: nextFrequency,
        status: "success",
      });
    } else {
      console.log("No available frequencies found matching the criteria.");
      // Zwróć 404, jeśli nie znaleziono ŻADNEJ wolnej częstotliwości (nawet bez preferencji)
      // lub jeśli nie znaleziono częstotliwości >= preferowanej
      res.status(404).json({
        error: "No available frequencies found matching the criteria",
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
