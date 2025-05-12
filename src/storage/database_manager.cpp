#include "database_manager.h"
#include <QDebug>
#include <cstdlib>

DatabaseManager::DatabaseManager(QObject *parent): QObject(parent), is_connected_(false) {
    try {
        const char *db_user = std::getenv("DB_USER");
        const char *db_name = std::getenv("DB_NAME");
        const char *db_password = std::getenv("DB_PASSWORD");
        const char *db_host = std::getenv("DB_HOST");
        const char *db_port = std::getenv("DB_PORT");
        const char *db_sslmode = std::getenv("DB_SSLMODE");
        const char *db_app_name = std::getenv("DB_APP_NAME");

        if (!db_user || !db_name || !db_password || !db_host || !db_port || !db_sslmode || !db_app_name) {
            qDebug() << "[DATABASE MANAGER] One or more database connection environment variables are not set.";
            is_connected_ = false;
            return;
        }

        std::string connection_string =
                "user=" + std::string(db_user) +
                " dbname=" + std::string(db_name) +
                " password=" + std::string(db_password) +
                " host=" + std::string(db_host) +
                " port=" + std::string(db_port) +
                " sslmode=" + std::string(db_sslmode) +
                " application_name=" + std::string(db_app_name);

        connection_ = std::make_unique<pqxx::connection>(connection_string);

        if (connection_->is_open()) {
            is_connected_ = true;

            pqxx::work txn{*connection_};
            const pqxx::result result = txn.exec(
                "SELECT EXISTS (SELECT FROM information_schema.tables "
                "WHERE table_name = 'active_wavelengths')"
            );
            const bool table_exists = result[0][0].as<bool>();
            if (!table_exists) {
                qDebug() << "[DATABASE MANAGER] Table 'active_wavelengths' does not exist in the database!";
            }
            txn.commit();
        } else {
            qDebug() << "[DATABASE MANAGER] Failed to connect to PostgreSQL";
            is_connected_ = false;
        }
    } catch (const std::exception &e) {
        qDebug() << "[DATABASE MANAGER] Failed to connect to PostgreSQL:" << e.what();
        is_connected_ = false;
    }
}
