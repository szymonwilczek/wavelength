const { Pool } = require("pg");
const path = require("path");
require("dotenv").config({ path: path.resolve(__dirname, "../.env") });

const connectionString =
    process.env.DATABASE_URL ||
    "postgresql://u_f67b73d1_c584_40b2_a189_3cf401949c75:NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2@pg.rapidapp.io:5433/db_f67b73d1_c584_40b2_a189_3cf401949c75?ssl=true&sslmode=no-verify&application_name=rapidapp_nodejs";

const pool = new Pool({
  connectionString,
  ssl: {
    rejectUnauthorized: false,
  },
});

pool.on("error", (err) => {
  console.error("Unexpected error on idle client", err);
});

/**
 * Runs a SQL query with parameters
 * @param {string} text - SQL query string
 * @param {Array} params - Array of parameters for the query
 * @returns {Promise} - Promise resolving to the result of the query
 * @throws {Error} - Throws an error if the query fails
 */
async function query(text, params) {
  const start = Date.now();
  try {
    const res = await pool.query(text, params);
    const duration = Date.now() - start;
    console.log("Executed query", { text, duration, rows: res.rowCount });
    return res;
  } catch (error) {
    console.error("Error executing query", { text, error });
    throw error;
  }
}

/**
 * Initializes the database by creating the necessary tables if they do not exist
 */
async function initDb() {
  try {
    // Drop existing table if necessary during development/testing
    // await query(`DROP TABLE IF EXISTS active_wavelengths;`);

    await query(`
      CREATE TABLE IF NOT EXISTS active_wavelengths (
        id SERIAL PRIMARY KEY, -- Keep serial ID
        frequency TEXT UNIQUE NOT NULL, -- Changed to TEXT UNIQUE
        name VARCHAR(255) NOT NULL,
        is_password_protected BOOLEAN NOT NULL DEFAULT false,
        host_socket_id VARCHAR(50) NOT NULL,
        password_hash VARCHAR(64), -- Added password hash column
        created_at TIMESTAMP NOT NULL DEFAULT NOW()
      )
    `);
    await query(`
      CREATE INDEX IF NOT EXISTS idx_active_wavelengths_frequency
        ON active_wavelengths(frequency);
    `);
    console.log("Database initialized successfully (frequency as TEXT)");
  } catch (error) {
    console.error("Error initializing database:", error);
    throw error;
  }
}

module.exports = {
  pool,
  query,
  initDb,
};
