// filepath: c:\Users\szymo\Documents\GitHub\wavelength\server\setup-db.js
const { Pool } = require('pg');
require("dotenv").config(); // Load .env variables

// Use environment variable for connection string if available
const connectionString = process.env.DATABASE_URL || "postgresql://u_f67b73d1_c584_40b2_a189_3cf401949c75:NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2@pg.rapidapp.io:5433/db_f67b73d1_c584_40b2_a189_3cf401949c75?ssl=true&sslmode=no-verify&application_name=rapidapp_setup";

const pool = new Pool({
  connectionString,
  ssl: {
    rejectUnauthorized: false // Adjust based on your SSL requirements
  }
});

async function setupDatabase() {
  const client = await pool.connect();
  try {
    console.log("Starting database setup...");
    // Drop existing table if necessary during development/testing
    // console.log("Dropping existing table (if exists)...");
    // await client.query(`DROP TABLE IF EXISTS active_wavelengths;`);

    // Utwórz tabelę
    console.log("Creating 'active_wavelengths' table (if not exists)...");
    await client.query(`
      CREATE TABLE IF NOT EXISTS active_wavelengths (
        id SERIAL PRIMARY KEY,
        frequency TEXT UNIQUE NOT NULL, -- Changed to TEXT UNIQUE
        name VARCHAR(255) NOT NULL,
        is_password_protected BOOLEAN DEFAULT FALSE,
        host_socket_id VARCHAR(255) NOT NULL,
        password_hash VARCHAR(64),
        created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
      );
    `);
    console.log("Table 'active_wavelengths' created or already exists (frequency as TEXT)");

    // Utwórz indeksy (index on UNIQUE column might be automatic, but explicit is fine)
    console.log("Creating index 'idx_active_wavelengths_frequency' (if not exists)...");
    await client.query(`
      CREATE INDEX IF NOT EXISTS idx_active_wavelengths_frequency
        ON active_wavelengths(frequency);
    `);
    console.log("Index 'idx_active_wavelengths_frequency' created or already exists");

    console.log("Database setup completed successfully");
  } catch (error) {
    console.error("Database setup failed:", error);
  } finally {
    client.release();
    await pool.end();
    console.log("Database connection pool closed.");
  }
}

setupDatabase();