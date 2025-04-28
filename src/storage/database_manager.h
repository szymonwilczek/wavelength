#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>
#include <pqxx/pqxx>

#include "../util/wavelength_utilities.h"

class DatabaseManager : public QObject {
    Q_OBJECT

public:
    static DatabaseManager* getInstance() {
        static DatabaseManager instance;
        return &instance;
    }

    bool isConnected() const {
        return m_isConnected;
    }

private:
    DatabaseManager(QObject* parent = nullptr);

    ~DatabaseManager() {}

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    std::unique_ptr<pqxx::connection> m_connection;
    bool m_isConnected;
};

#endif // DATABASE_MANAGER_H