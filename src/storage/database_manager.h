#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>
#include <pqxx/pqxx>

class DatabaseManager final : public QObject {
    Q_OBJECT

public:
    static DatabaseManager* GetInstance() {
        static DatabaseManager instance;
        return &instance;
    }

    bool IsConnected() const {
        return is_connected_;
    }

private:
    explicit DatabaseManager(QObject* parent = nullptr);

    ~DatabaseManager() override {}

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    std::unique_ptr<pqxx::connection> connection_;
    bool is_connected_;
};

#endif // DATABASE_MANAGER_H