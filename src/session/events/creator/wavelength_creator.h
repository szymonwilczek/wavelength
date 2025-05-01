#ifndef WAVELENGTH_CREATOR_H
#define WAVELENGTH_CREATOR_H

#include "../../../storage/database_manager.h"
#include "../../../chat/messages/services/wavelength_message_processor.h"
#include "../../../../src/app/wavelength_config.h"

/**
 * @brief Singleton class responsible for initiating the creation of a new wavelength.
 *
 * This class handles the process of connecting to the relay server, sending a
 * registration request for a new frequency (wavelength), and managing the initial
 * WebSocket connection until the registration result is received. It interacts with
 * WavelengthRegistry, WavelengthConfig, AuthenticationManager, and MessageHandler.
 * It emits signals indicating success (wavelengthCreated), failure (connectionError),
 * or closure (wavelengthClosed).
 */
class WavelengthCreator final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthCreator.
     * @return Pointer to the singleton WavelengthCreator instance.
     */
    static WavelengthCreator* GetInstance() {
        static WavelengthCreator instance;
        return &instance;
    }

    /**
     * @brief Attempts to create and register a new wavelength with the relay server.
     * Checks if the wavelength already exists locally or is pending registration.
     * Creates a new WebSocket connection, connects signals for connection events, errors,
     * and the registration result message. Sends the registration request upon successful connection.
     * Manages a keep-alive timer for the connection.
     * @param frequency The desired name for the new wavelength.
     * @param is_password_protected True if the wavelength should require a password.
     * @param password The password to set (if protected).
     * @return True if the creation process was initiated (connection attempt started),
     *         false if the wavelength already exists or is pending.
     */
    bool CreateWavelength(QString frequency, bool is_password_protected,
                  const QString& password);

signals:
    /**
     * @brief Emitted when the wavelength has been successfully created and registered with the server.
     * @param frequency The frequency identifier of the newly created wavelength.
     */
    void wavelengthCreated(QString frequency);

    /**
     * @brief Emitted if the WebSocket connection for the wavelength is closed unexpectedly
     *        (e.g., during registration or after creation).
     * @param frequency The frequency identifier of the wavelength whose connection closed.
     */
    void wavelengthClosed(QString frequency);

    /**
     * @brief Emitted if an error occurs during the connection or registration process.
     * @param error_message A description of the error.
     */
    void connectionError(const QString& error_message);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit WavelengthCreator(QObject* parent = nullptr) : QObject(parent) {}

    /**
     * @brief Private destructor.
     */
    ~WavelengthCreator() override = default; // Use default destructor

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    WavelengthCreator(const WavelengthCreator&) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    WavelengthCreator& operator=(const WavelengthCreator&) = delete;
};

#endif // WAVELENGTH_CREATOR_H