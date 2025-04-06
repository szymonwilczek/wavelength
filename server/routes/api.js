/**
 * Router API dla aplikacji Wavelength
 */
const express = require("express");
const wavelengthService = require("../services/wavelengthService");
const connectionManager = require("../websocket/connectionManager");
const { normalizeFrequency } = require("../utils/helpers");

const router = express.Router();

/**
 * Endpoint sprawdzający stan serwera
 * @route GET /health
 */
router.get("/health", (req, res) => {
  res.status(200).json({ status: "ok" });
});

/**
 * Zwraca następną dostępną częstotliwość
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
 * Zwraca listę wszystkich aktywnych wavelengths
 * @route GET /api/wavelengths
 */
router.get("/api/wavelengths", async (req, res) => {
  try {
    const wavelengths = await wavelengthService.getAllWavelengths();

    // Przygotuj dane do odpowiedzi
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
 * Zwraca informacje o określonej częstotliwości
 * @route GET /api/wavelengths/:frequency
 */
router.get("/api/wavelengths/:frequency", async (req, res) => {
  try {
    const frequency = normalizeFrequency(req.params.frequency);

    // Sprawdź najpierw w pamięci
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

    // Jeśli nie ma w pamięci, sprawdź w bazie danych
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

    // Nie znaleziono
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
