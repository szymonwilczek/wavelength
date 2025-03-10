const express = require("express");
const http = require("http");
const { Server } = require("socket.io");
const { MongoClient } = require("mongodb");

const PORT = process.env.PORT || 3000;
const MONGO_URI = process.env.MONGO_URI;
const DB_NAME = "wavelengthDB";

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"],
  },
});

let db;
MongoClient.connect(MONGO_URI)
  .then((client) => {
    console.log("Connected to MongoDB");
    db = client.db(DB_NAME);

    return db.collection("activeWavelengths").deleteMany({});
  })
  .then(() => {
    console.log("Active wavelengths collection cleared");
  })
  .catch((err) => {
    console.error("MongoDB connection error:", err);
    process.exit(1);
  });

const activeSessions = new Map(); // frequency -> { host, clients, settings }

app.get("/health", (req, res) => {
  res.status(200).json({ status: "ok" });
});

io.on("connection", (socket) => {
  console.log(`Client connected: ${socket.id}`);

  socket.on("register_wavelength", async (data) => {
    try {
      const { frequency, name, isPasswordProtected } = data;

      const existingWavelength = await db
        .collection("activeWavelengths")
        .findOne({ frequency });
      if (existingWavelength) {
        socket.emit("register_result", {
          success: false,
          error: "Frequency is already in use",
        });
        return;
      }

      const sessionId = socket.id;
      socket.join(`wavelength_${frequency}`);

      activeSessions.set(frequency, {
        host: socket.id,
        clients: new Set(),
        settings: {
          name,
          isPasswordProtected,
          frequency,
        },
      });

      await db.collection("activeWavelengths").insertOne({
        frequency,
        name,
        hostSocketId: socket.id,
        isPasswordProtected,
        createdAt: new Date(),
      });

      socket.data.frequency = frequency;
      socket.data.isHost = true;

      socket.emit("register_result", {
        success: true,
        sessionId,
        frequency,
      });

      console.log(`New wavelength registered: ${frequency}Hz by ${socket.id}`);
    } catch (err) {
      console.error("Error registering wavelength:", err);
      socket.emit("register_result", {
        success: false,
        error: "Server error, please try again",
      });
    }
  });

  socket.on("join_wavelength", async (data) => {
    try {
      const { frequency, password } = data;

      const wavelength = await db
        .collection("activeWavelengths")
        .findOne({ frequency });
      if (!wavelength) {
        socket.emit("join_result", {
          success: false,
          error: "Wavelength does not exist",
        });
        return;
      }

      if (wavelength.isPasswordProtected && password !== data.password) {
        socket.emit("join_result", {
          success: false,
          error: "Invalid password",
        });
        return;
      }

      socket.join(`wavelength_${frequency}`);

      socket.data.frequency = frequency;
      socket.data.isHost = false;

      const session = activeSessions.get(frequency);
      if (session) {
        session.clients.add(socket.id);
      }

      const hostSocket = io.sockets.sockets.get(wavelength.hostSocketId);
      if (hostSocket) {
        hostSocket.emit("client_joined", {
          clientId: socket.id,
          frequency,
        });
      }

      socket.emit("join_result", {
        success: true,
        frequency,
        name: wavelength.name,
      });

      console.log(`Client ${socket.id} joined wavelength ${frequency}Hz`);
    } catch (err) {
      console.error("Error joining wavelength:", err);
      socket.emit("join_result", {
        success: false,
        error: "Server error, please try again",
      });
    }
  });

  socket.on("send_message", (data) => {
    const { message } = data;
    const frequency = socket.data.frequency;

    if (!frequency) {
      socket.emit("error", { message: "Not connected to any wavelength" });
      return;
    }

    const isHost = socket.data.isHost;
    const sender = isHost ? "Host" : socket.id.substring(0, 8);

    io.to(`wavelength_${frequency}`).emit("message", {
      content: message,
      sender,
      frequency,
      timestamp: new Date().toISOString(),
    });

    console.log(`Message in wavelength ${frequency}Hz from ${sender}`);
  });

  socket.on("close_wavelength", async () => {
    const frequency = socket.data.frequency;

    if (!frequency || !socket.data.isHost) {
      socket.emit("error", {
        message: "Not authorized to close this wavelength",
      });
      return;
    }

    try {
      io.to(`wavelength_${frequency}`).emit("wavelength_closed", {
        frequency,
        reason: "Host closed the wavelength",
      });

      await db.collection("activeWavelengths").deleteOne({ frequency });

      activeSessions.delete(frequency);

      console.log(`Wavelength ${frequency}Hz closed by host ${socket.id}`);
    } catch (err) {
      console.error("Error closing wavelength:", err);
      socket.emit("error", { message: "Failed to close wavelength" });
    }
  });

  socket.on("disconnect", async () => {
    const frequency = socket.data.frequency;
    const isHost = socket.data.isHost;

    console.log(
      `Client disconnected: ${socket.id}, was ${
        isHost ? "host" : "client"
      } of wavelength ${frequency}`
    );

    if (!frequency) return;

    if (isHost) {
      try {
        io.to(`wavelength_${frequency}`).emit("wavelength_closed", {
          frequency,
          reason: "Host disconnected",
        });

        await db.collection("activeWavelengths").deleteOne({ frequency });

        activeSessions.delete(frequency);

        console.log(`Wavelength ${frequency}Hz closed due to host disconnect`);
      } catch (err) {
        console.error("Error processing host disconnect:", err);
      }
    } else {
      const session = activeSessions.get(frequency);
      if (session) {
        session.clients.delete(socket.id);

        const hostSocket = io.sockets.sockets.get(session.host);
        if (hostSocket) {
          hostSocket.emit("client_left", {
            clientId: socket.id,
            frequency,
          });
        }
      }
    }
  });

  socket.on("check_frequency", async (data) => {
    try {
      const { frequency } = data;
      const existingWavelength = await db
        .collection("activeWavelengths")
        .findOne({ frequency });

      socket.emit("frequency_result", {
        frequency,
        available: !existingWavelength,
      });
    } catch (err) {
      console.error("Error checking frequency:", err);
      socket.emit("frequency_result", {
        frequency: data.frequency,
        error: "Server error",
      });
    }
  });
});

server.listen(PORT, () => {
  console.log(`Wavelength relay server running on port ${PORT}`);
});
