const { Pool } = require('pg');

const connectionString = "postgresql://u_f67b73d1_c584_40b2_a189_3cf401949c75:NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2@pg.rapidapp.io:5433/db_f67b73d1_c584_40b2_a189_3cf401949c75?ssl=true&sslmode=no-verify&application_name=rapidapp_setup";

const pool = new Pool({
  connectionString,
  ssl: {
    rejectUnauthorized: false
  }
});

async function setupDatabase() {
  try {
    // Utwórz tabelę
    await pool.query(`
      CREATE TABLE IF NOT EXISTS active_wavelengths (
                                                      id SERIAL PRIMARY KEY,
                                                      frequency NUMERIC(12, 1) UNIQUE NOT NULL,
        name VARCHAR(255) NOT NULL,
        is_password_protected BOOLEAN DEFAULT FALSE,
        host_socket_id VARCHAR(255) NOT NULL,
        password_hash VARCHAR(64),
        created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
                               );
    `);
    console.log("Table 'active_wavelengths' created or already exists");

    // Utwórz indeksy
    await pool.query(`
      CREATE INDEX IF NOT EXISTS idx_active_wavelengths_frequency
        ON active_wavelengths(frequency);
    `);
    console.log("Index created or already exists");

    console.log("Database setup completed successfully");
  } catch (error) {
    console.error("Setup failed:", error);
  } finally {
    await pool.end();
  }
}

setupDatabase();