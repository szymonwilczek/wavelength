#ifndef WAVELENGTH_REGISTRY_H
#define WAVELENGTH_REGISTRY_H

#include <QObject>
#include <QString>
#include <QPointer>
#include <QWebSocket>
#include <QDateTime>
#include <QDebug>

/**
 * @brief Structure holding information about a specific wavelength (frequency).
 *
 * Contains details like password protection status, host information, the associated
 * WebSocket connection, and state flags.
 */
struct WavelengthInfo {
    /** @brief The unique identifier (name) of the wavelength. */
    QString frequency;
    /** @brief True if the wavelength requires a password to join. */
    bool is_password_protected = false;
    /** @brief The password for the wavelength (if protected). */
    QString password;
    /** @brief The unique client ID of the user hosting the wavelength. */
    QString host_id;
    /** @brief True if the current user is the host of this wavelength. */
    bool is_host = false;
    /** @brief The port number the host is listening on (potentially unused with relay). */
    int host_port = 0;
    /** @brief QPointer to the WebSocket connection associated with this wavelength. Automatically nullifies if the socket is deleted. */
    QPointer<QWebSocket> socket = nullptr;
    /** @brief Flag indicating if the wavelength is currently in the process of being closed. */
    bool is_closing = false;
    /** @brief Timestamp when this wavelength information was created or added to the registry locally. */
    QDateTime created_at = QDateTime::currentDateTime();
};

/**
 * @brief Singleton registry managing active and pending wavelength connections and their associated information.
 *
 * This class acts as a central repository for all known wavelengths (frequencies) that the
 * application is aware of, either joined, hosted, or pending registration. It stores
 * WavelengthInfo for each frequency and manages the concept of an "active" wavelength.
 * It provides methods for adding, removing, updating, and querying wavelength information
 * and their associated WebSocket connections. It emits signals when the registry state changes.
 */
class WavelengthRegistry final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthRegistry.
     * @return Pointer to the singleton WavelengthRegistry instance.
     */
    static WavelengthRegistry *GetInstance() {
        static WavelengthRegistry instance;
        return &instance;
    }

    /**
     * @brief Adds a new wavelength and its information to the registry.
     * Removes any pending registration for the same frequency. Emits wavelengthAdded signal.
     * @param frequency The unique identifier for the new wavelength.
     * @param info The WavelengthInfo struct containing details about the wavelength.
     * @return True if added successfully, false if the frequency already exists.
     */
    bool AddWavelength(const QString &frequency, const WavelengthInfo &info);

    /**
     * @brief Removes a wavelength from the registry.
     * If the removed wavelength was the active one, sets the active wavelength to "-1".
     * Emits wavelengthRemoved signal.
     * @param frequency The identifier of the wavelength to remove.
     * @return True if removed successfully, false if the frequency does not exist.
     */
    bool RemoveWavelength(const QString &frequency);

    /**
     * @brief Checks if a wavelength with the given frequency exists in the registry.
     * @param frequency The frequency identifier to check.
     * @return True if the wavelength exists, false otherwise.
     */
    bool HasWavelength(const QString &frequency) const {
        return wavelengths_.contains(frequency);
    }

    /**
     * @brief Retrieves the WavelengthInfo for a specific frequency.
     * @param frequency The frequency identifier.
     * @return The WavelengthInfo struct. Returns a default-constructed WavelengthInfo if the frequency is not found.
     */
    WavelengthInfo GetWavelengthInfo(const QString &frequency) const;

    /**
     * @brief Gets a list of all frequency identifiers currently in the registry.
     * @return A QList<QString> containing all known frequencies.
     */
    QList<QString> GetAllWavelengths() const {
        return wavelengths_.keys();
    }

    /**
     * @brief Updates the information for an existing wavelength.
     * Emits wavelengthUpdated signal.
     * @param frequency The identifier of the wavelength to update.
     * @param info The new WavelengthInfo struct.
     * @return True if updated successfully, false if the frequency does not exist.
     */
    bool UpdateWavelength(const QString &frequency, const WavelengthInfo &info);

    /**
     * @brief Sets the specified frequency as the currently active wavelength.
     * If the frequency does not exist in the registry (and is not "-1"), the operation is ignored.
     * Emits activeWavelengthChanged signal.
     * @param frequency The frequency identifier to set as active, or "-1" for none.
     */
    void SetActiveWavelength(const QString &frequency);

    /**
     * @brief Gets the identifier of the currently active wavelength.
     * @return The frequency string, or "-1" if no wavelength is active.
     */
    QString GetActiveWavelength() const {
        return active_wavelength_;
    }

    /**
     * @brief Checks if the specified frequency is the currently active one.
     * @param frequency The frequency identifier to check.
     * @return True if the given frequency is the active one, false otherwise.
     */
    bool IsWavelengthActive(const QString &frequency) const {
        return active_wavelength_ == frequency;
    }

    /**
     * @brief Marks a frequency as having a pending registration attempt.
     * Used to prevent duplicate registration attempts while one is in progress.
     * @param frequency The frequency identifier.
     * @return True if marked successfully, false if the frequency already exists or is already pending.
     */
    bool AddPendingRegistration(const QString &frequency);

    /**
     * @brief Removes a frequency from the pending registration list.
     * @param frequency The frequency identifier.
     * @return True if removed successfully, false if it was not pending.
     */
    bool RemovePendingRegistration(const QString &frequency);

    /**
     * @brief Checks if a frequency is currently marked as having a pending registration.
     * @param frequency The frequency identifier.
     * @return True if registration is pending, false otherwise.
     */
    bool IsPendingRegistration(const QString &frequency) const {
        return pending_registrations_.contains(frequency);
    }

    /**
     * @brief Retrieves the WebSocket connection associated with a specific frequency.
     * @param frequency The frequency identifier.
     * @return A QPointer<QWebSocket> to the socket, or nullptr if the frequency doesn't exist or has no socket.
     */
    QPointer<QWebSocket> GetWavelengthSocket(const QString &frequency) const;

    /**
     * @brief Sets or updates the WebSocket connection associated with a specific frequency.
     * @param frequency The frequency identifier.
     * @param socket A QPointer<QWebSocket> to the socket.
     */
    void SetWavelengthSocket(const QString &frequency, const QPointer<QWebSocket> &socket);

    /**
     * @brief Marks or unmarks a wavelength as being in the process of closing.
     * @param frequency The frequency identifier.
     * @param closing True to mark as closing, false otherwise.
     * @return True if the status was updated, false if the frequency does not exist.
     */
    bool MarkWavelengthClosing(const QString &frequency, bool closing = true);

    /**
     * @brief Checks if a wavelength is currently marked as closing.
     * @param frequency The frequency identifier.
     * @return True if marked as closing, false otherwise or if the frequency doesn't exist.
     */
    bool IsWavelengthClosing(const QString &frequency) const;

    /**
     * @brief Removes all wavelengths and pending registrations from the registry.
     * Sets the active wavelength to "-1".
     */
    void ClearAllWavelengths();

signals:
    /**
     * @brief Emitted when a new wavelength is successfully added to the registry.
     * @param frequency The identifier of the added wavelength.
     */
    void wavelengthAdded(QString frequency);

    /**
     * @brief Emitted when a wavelength is successfully removed from the registry.
     * @param frequency The identifier of the removed wavelength.
     */
    void wavelengthRemoved(QString frequency);

    /**
     * @brief Emitted when the information for an existing wavelength is updated.
     * @param frequency The identifier of the updated wavelength.
     */
    void wavelengthUpdated(QString frequency);

    /**
     * @brief Emitted when the active wavelength changes via SetActiveWavelength().
     * @param previous_frequency The identifier of the previously active wavelength ("-1" if none).
     * @param new_frequency The identifier of the newly active wavelength ("-1" if none).
     */
    void activeWavelengthChanged(QString previous_frequency, QString new_frequency);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern. Initializes active_wavelength_ to "-1".
     * @param parent Optional parent QObject.
     */
    explicit WavelengthRegistry(QObject *parent = nullptr) : QObject(parent), active_wavelength_("-1") {
    } // Corrected default active_wavelength_

    /**
     * @brief Private destructor.
     */
    ~WavelengthRegistry() override = default;

    /** @brief Deleted copy constructor to prevent copying. */
    WavelengthRegistry(const WavelengthRegistry &) = delete;

    /** @brief Deleted assignment operator to prevent assignment. */
    WavelengthRegistry &operator=(const WavelengthRegistry &) = delete;

    /** @brief Map storing WavelengthInfo for each registered frequency. Key is the frequency string. */
    QMap<QString, WavelengthInfo> wavelengths_;
    /** @brief Set storing frequencies for which a registration attempt is currently in progress. */
    QSet<QString> pending_registrations_;
    /** @brief The identifier of the currently active wavelength, or "-1" if none is active. */
    QString active_wavelength_;
};

#endif // WAVELENGTH_REGISTRY_H
