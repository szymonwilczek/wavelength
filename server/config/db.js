const { Pool } = require("pg");
const path = require("path");
require("dotenv").config({ path: path.resolve(__dirname, "../.env") });

// Użycie bezpośredniego connection string
const connectionString =
  process.env.DATABASE_URL ||
  "postgresql://u_f67b73d1_c584_40b2_a189_3cf401949c75:NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2@pg.rapidapp.io:5433/db_f67b73d1_c584_40b2_a189_3cf401949c75?ssl=true&sslmode=no-verify&application_name=rapidapp_nodejs";

// Konfiguracja połączenia z bazą danych
const pool = new Pool({
  connectionString,
  ssl: {
    rejectUnauthorized: false,
  },
});

// Nasłuchiwanie na błędy połączenia
pool.on("error", (err) => {
  console.error("Unexpected error on idle client", err);
});

/**
 * Uruchamia zapytanie SQL z parametrami
 * @param {string} text - Zapytanie SQL
 * @param {Array} params - Parametry zapytania
 * @returns {Promise} - Wynik zapytania
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
 * Inicjalizuje tabelę active_wavelengths jeśli nie istnieje
 */
async function initDb() {
  try {
    await query(`
      CREATE TABLE IF NOT EXISTS active_wavelengths (
        frequency DECIMAL(10, 1) PRIMARY KEY,
        name VARCHAR(255) NOT NULL,
        is_password_protected BOOLEAN NOT NULL DEFAULT false,
        host_socket_id VARCHAR(50) NOT NULL,
        created_at TIMESTAMP NOT NULL DEFAULT NOW()
      )
    `);
    console.log("Database initialized successfully");
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
