#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <pqxx/pqxx>  // Upewnij się, że masz zainstalowaną libpqxx z obsługą SSL

#include "../util/wavelength_utilities.h"

class DatabaseManager : public QObject {
    Q_OBJECT

public:
    static DatabaseManager* getInstance() {
        static DatabaseManager instance;
        return &instance;
    }

    bool isFrequencyAvailable(double frequency) {
        try {
            double normalizedFrequency = WavelengthUtilities::normalizeFrequency(frequency);

            pqxx::work txn{*m_connection};
            pqxx::result result = txn.exec_params(
                "SELECT COUNT(*) FROM active_wavelengths WHERE CAST(frequency AS NUMERIC(12,1)) = CAST($1 AS NUMERIC(12,1))",
                normalizedFrequency
            );
            txn.commit();

            int count = result[0][0].as<int>();
            qDebug() << "Frequency" << normalizedFrequency << "availability check: "
                     << (count == 0 ? "available" : "taken");
            return count == 0;
        }
        catch (const std::exception& e) {
            qDebug() << "PostgreSQL error when checking frequency availability:" << e.what();
            return false;
        }
    }

    bool getWavelengthDetails(double frequency, QString& name, bool& isPasswordProtected) {
        try {
            // Normalizacja częstotliwości
            double normalizedFrequency = WavelengthUtilities::normalizeFrequency(frequency);

            qDebug() << "Executing SQL query for frequency:" << normalizedFrequency;

            // Używamy CAST do zapewnienia zgodności typów
            pqxx::work txn{*m_connection};
            pqxx::result result = txn.exec_params(
                "SELECT name, is_password_protected FROM active_wavelengths WHERE CAST(frequency AS NUMERIC(12,1)) = CAST($1 AS NUMERIC(12,1))",
                normalizedFrequency
            );
            txn.commit();

            if (result.empty()) {
                qDebug() << "Wavelength" << normalizedFrequency << "not found in database";
                return false;
            }

            name = QString::fromStdString(result[0]["name"].as<std::string>());
            isPasswordProtected = result[0]["is_password_protected"].as<bool>();

            qDebug() << "Found wavelength in database:" << normalizedFrequency
                     << "Name:" << name << "Protected:" << isPasswordProtected;

            return true;
        }
        catch (const std::exception& e) {
            qDebug() << "PostgreSQL error when getting wavelength details:" << e.what();
            return false;
        }
    }

    bool isConnected() const {
        return m_isConnected;
    }

private:
    DatabaseManager(QObject* parent = nullptr)
        : QObject(parent), m_isConnected(false) {
        try {
            // Parametry połączenia do hostowanej bazy PostgreSQL
            std::string connectionString =
                "user=u_f67b73d1_c584_40b2_a189_3cf401949c75 "
                "dbname=db_f67b73d1_c584_40b2_a189_3cf401949c75 "
                "password=NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2 "
                "host=pg.rapidapp.io "
                "port=5433 "
                "sslmode=require "
                "application_name=rapidapp_cpp";

            m_connection = std::make_unique<pqxx::connection>(connectionString);

            if (m_connection->is_open()) {
                m_isConnected = true;
                qDebug() << "Connected to PostgreSQL successfully";

                // Sprawdź czy tabela istnieje, jeśli nie - wyświetl ostrzeżenie
                pqxx::work txn{*m_connection};
                pqxx::result result = txn.exec(
                    "SELECT EXISTS (SELECT FROM information_schema.tables "
                    "WHERE table_name = 'active_wavelengths')"
                );
                bool tableExists = result[0][0].as<bool>();
                if (!tableExists) {
                    qDebug() << "WARNING: Table 'active_wavelengths' does not exist in the database!";
                }
                txn.commit();
            } else {
                qDebug() << "Failed to connect to PostgreSQL";
                m_isConnected = false;
            }
        }
        catch (const std::exception& e) {
            qDebug() << "Failed to connect to PostgreSQL:" << e.what();
            m_isConnected = false;
        }
    }

    ~DatabaseManager() {}

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    std::unique_ptr<pqxx::connection> m_connection;
    bool m_isConnected;
};

#endif // DATABASE_MANAGER_H