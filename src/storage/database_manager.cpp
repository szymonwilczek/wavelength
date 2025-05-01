#include "database_manager.h"
#include <QDebug>

DatabaseManager::DatabaseManager(QObject *parent): QObject(parent), is_connected_(false) {
    try {
        // Parametry połączenia do hostowanej bazy PostgreSQL
        std::string connection_string =
                "user=u_f67b73d1_c584_40b2_a189_3cf401949c75 "
                "dbname=db_f67b73d1_c584_40b2_a189_3cf401949c75 "
                "password=NUlV9202u7L7J8i9sVIk6hC8erKY2O5v5v72s0v3nJ1hyy6QsnA2 "
                "host=pg.rapidapp.io "
                "port=5433 "
                "sslmode=require "
                "application_name=rapidapp_cpp";

        connection_ = std::make_unique<pqxx::connection>(connection_string);

        if (connection_->is_open()) {
            is_connected_ = true;
            qDebug() << "Connected to PostgreSQL successfully";

            // Sprawdź czy tabela istnieje, jeśli nie - wyświetl ostrzeżenie
            pqxx::work txn{*connection_};
            const pqxx::result result = txn.exec(
                "SELECT EXISTS (SELECT FROM information_schema.tables "
                "WHERE table_name = 'active_wavelengths')"
            );
            const bool table_exists = result[0][0].as<bool>();
            if (!table_exists) {
                qDebug() << "WARNING: Table 'active_wavelengths' does not exist in the database!";
            }
            txn.commit();
        } else {
            qDebug() << "Failed to connect to PostgreSQL";
            is_connected_ = false;
        }
    }
    catch (const std::exception& e) {
        qDebug() << "Failed to connect to PostgreSQL:" << e.what();
        is_connected_ = false;
    }
}
