#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <pqxx/pqxx>

/**
 * @brief Singleton class responsible for managing the connection to the PostgreSQL database.
 *
 * This class establishes and holds the connection to the external PostgreSQL database
 * using the pqxx library. It provides a method to check the connection status.
 * The connection parameters are hardcoded within the constructor.
 */
class DatabaseManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the DatabaseManager.
     * @return Pointer to the singleton DatabaseManager instance.
     */
    static DatabaseManager *GetInstance() {
        static DatabaseManager instance;
        return &instance;
    }

    /**
     * @brief Checks if the connection to the database was successfully established.
     * @return True if connected, false otherwise.
     */
    bool IsConnected() const {
        return is_connected_;
    }

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * Attempts to establish the connection to the PostgreSQL database using hardcoded credentials.
     * Sets the is_connected_ flag based on the outcome. Checks for the existence of the
     * 'active_wavelengths' table upon successful connection.
     * @param parent Optional parent QObject.
     */
    explicit DatabaseManager(QObject *parent = nullptr);

    /**
     * @brief Private destructor.
     * The unique_ptr automatically manages the pqxx::connection lifetime.
     */
    ~DatabaseManager() override = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    DatabaseManager(const DatabaseManager &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    /** @brief Unique pointer managing the pqxx database connection object. */
    std::unique_ptr<pqxx::connection> connection_;
    /** @brief Flag indicating whether the database connection is currently established. */
    bool is_connected_;
};

#endif // DATABASE_MANAGER_H
