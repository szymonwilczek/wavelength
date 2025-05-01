#ifndef WAVELENGTH_STATE_MANAGER_H
#define WAVELENGTH_STATE_MANAGER_H

#include <QObject>
#include <QString>
#include <QList>

#include "../storage/wavelength_registry.h"

/**
 * @brief Singleton class providing a centralized interface for querying and managing the state of wavelengths.
 *
 * This class acts as a facade over WavelengthRegistry and adds its own state tracking
 * (like joined wavelengths, connecting status, and session data). It provides convenient
 * methods to access information about specific wavelengths (info, host status, password protection,
 * connection status) and the overall application state (active wavelength, joined wavelengths).
 * It also manages temporary session data associated with specific frequencies.
 */
class WavelengthStateManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthStateManager.
     * @return Pointer to the singleton WavelengthStateManager instance.
     */
    static WavelengthStateManager* GetInstance() {
        static WavelengthStateManager instance;
        return &instance;
    }

    /**
     * @brief Retrieves detailed information about a specific wavelength from the registry.
     * @param frequency The frequency identifier of the wavelength.
     * @param is_host Optional output parameter; if provided, it's set to true if the current user is the host of this wavelength.
     * @return A WavelengthInfo struct containing details about the wavelength.
     */
    static WavelengthInfo GetWavelengthInfo(const QString &frequency, bool* is_host = nullptr);

    /**
     * @brief Gets the frequency identifier of the currently active wavelength.
     * @return The frequency string, or "-1" if no wavelength is active.
     */
    static QString GetActiveWavelength();

    /**
     * @brief Sets the specified frequency as the currently active wavelength.
     * Updates the registry and emits the activeWavelengthChanged signal.
     * @param frequency The frequency identifier to set as active.
     */
    void SetActiveWavelength(const QString &frequency);

    /**
     * @brief Checks if the current user is the host of the currently active wavelength.
     * @return True if an active wavelength exists and the user is its host, false otherwise.
     */
    static bool IsActiveWavelengthHost();

    /**
     * @brief Gets a list of frequency identifiers for all wavelengths the user is currently joined to.
     * Relies on internal tracking combined with registry validation.
     * @return A QList<QString> containing the frequencies of joined wavelengths.
     */
    QList<QString> GetJoinedWavelengths();

    /**
     * @brief Registers that the user has successfully joined a specific wavelength.
     * Adds the frequency to an internal list if not already present.
     * @param frequency The frequency identifier of the joined wavelength.
     */
    void RegisterJoinedWavelength(const QString &frequency);

    /**
     * @brief Unregisters that the user has left or been disconnected from a specific wavelength.
     * Removes the frequency from the internal list.
     * @param frequency The frequency identifier of the wavelength being left.
     */
    void UnregisterJoinedWavelength(const QString &frequency);

    /**
     * @brief Gets the total number of wavelengths the user is currently joined to.
     * @return The count of joined wavelengths.
     */
    int GetJoinedWavelengthCount();

    /**
     * @brief Checks if a specific wavelength is password protected.
     * @param frequency The frequency identifier to check.
     * @return True if the wavelength requires a password, false otherwise.
     */
    static bool IsWavelengthPasswordProtected(const QString &frequency);

    /**
     * @brief Checks if the current user is the host of a specific wavelength.
     * @param frequency The frequency identifier to check.
     * @return True if the user is the host, false otherwise.
     */
    static bool IsWavelengthHost(const QString &frequency);

    /**
     * @brief Gets the creation timestamp of a specific wavelength.
     * @param frequency The frequency identifier.
     * @return The QDateTime when the wavelength was created/registered.
     */
    static QDateTime GetWavelengthCreationTime(const QString &frequency);

    /**
     * @brief Checks if the user is currently considered joined to a specific wavelength (based on registry presence).
     * @param frequency The frequency identifier to check.
     * @return True if the wavelength exists in the registry, false otherwise.
     */
    static bool IsWavelengthJoined(const QString &frequency);

    /**
     * @brief Checks if the WebSocket connection for a specific wavelength is currently established and valid.
     * @param frequency The frequency identifier to check.
     * @return True if the socket exists, is valid, and in the ConnectedState, false otherwise.
     */
    static bool IsWavelengthConnected(const QString &frequency);

    /**
     * @brief Stores arbitrary session data associated with a specific frequency and key.
     * Useful for temporary state related to a wavelength session (e.g., UI state).
     * @param frequency The frequency identifier.
     * @param key The key for the data item.
     * @param value The QVariant value to store.
     */
    void AddActiveSessionData(const QString &frequency, const QString& key, const QVariant& value);

    /**
     * @brief Retrieves previously stored session data for a specific frequency and key.
     * @param frequency The frequency identifier.
     * @param key The key of the data item to retrieve.
     * @param default_value Optional default value to return if the key or frequency is not found.
     * @return The stored QVariant value, or the default_value if not found.
     */
    QVariant GetActiveSessionData(const QString &frequency, const QString& key, const QVariant& default_value = QVariant());

    /**
     * @brief Removes all session data associated with a specific frequency.
     * @param frequency The frequency identifier whose session data should be cleared.
     */
    void ClearSessionData(const QString &frequency);

    /**
     * @brief Removes all session data for all frequencies.
     */
    void ClearAllSessionData();

    /**
     * @brief Checks if the application is currently in the process of connecting to a specific wavelength.
     * @param frequency The frequency identifier to check.
     * @return True if a connection attempt is in progress for this frequency, false otherwise.
     */
    bool IsConnecting(const QString &frequency) const;

    /**
     * @brief Sets or clears the "connecting" status for a specific wavelength.
     * Used to track ongoing connection attempts.
     * @param frequency The frequency identifier.
     * @param connecting True to mark as connecting, false to clear the status.
     */
    void SetConnecting(const QString &frequency, bool connecting);

signals:
    /**
     * @brief Emitted when the active wavelength changes via SetActiveWavelength().
     * @param frequency The frequency identifier of the newly active wavelength.
     */
    void activeWavelengthChanged(QString frequency);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit WavelengthStateManager(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Private destructor.
     */
    ~WavelengthStateManager() override = default; // Use default destructor

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    WavelengthStateManager(const WavelengthStateManager&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    WavelengthStateManager& operator=(const WavelengthStateManager&) = delete;

    /** @brief Stores temporary session data, keyed by frequency, then by data key. */
    QMap<QString, QMap<QString, QVariant>> session_data_;
    /** @brief List of frequencies currently undergoing a connection attempt. */
    QList<QString> connecting_wavelengths_;
    /** @brief List of frequencies the user is currently joined to (managed internally). */
    QList<QString> joined_wavelengths_;
};

#endif // WAVELENGTH_STATE_MANAGER_H