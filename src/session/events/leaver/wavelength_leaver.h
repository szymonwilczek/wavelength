#ifndef WAVELENGTH_LEAVER_H
#define WAVELENGTH_LEAVER_H

#include <QObject>

/**
 * @brief Singleton class responsible for handling the process of leaving or closing wavelengths.
 *
 * Provides methods to gracefully disconnect from the currently active wavelength
 * (LeaveWavelength) or to close a specific wavelength if the user is the host
 * (CloseWavelength). It interacts with WavelengthRegistry and MessageHandler to
 * send appropriate commands to the server and update the local state.
 */
class WavelengthLeaver final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Gets the singleton instance of the WavelengthLeaver.
     * @return Pointer to the singleton WavelengthLeaver instance.
     */
    static WavelengthLeaver *GetInstance() {
        static WavelengthLeaver instance;
        return &instance;
    }

    /**
     * @brief Leaves the currently active wavelength.
     * Retrieves the active frequency from the registry. If the user is the host,
     * sends a "close_wavelength" command; otherwise, sends a "leave_wavelength" command.
     * Closes the WebSocket connection, removes the wavelength from the registry,
     * sets the active wavelength to "-1", and emits the wavelengthLeft signal.
     */
    void LeaveWavelength();

    /**
     * @brief Closes a specific wavelength (only if the current user is the host).
     * Checks if the user is the host for the given frequency. Sends a "close_wavelength"
     * command to the server, closes the WebSocket connection, removes the wavelength
     * from the registry, and emits the wavelengthClosed signal. If the closed wavelength
     * was the active one, sets the active wavelength to "-1".
     * @param frequency The frequency identifier of the wavelength to close.
     */
    void CloseWavelength(QString frequency);

signals:
    /**
     * @brief Emitted after the user successfully leaves the active wavelength via LeaveWavelength().
     * @param frequency The frequency identifier of the wavelength that was left.
     */
    void wavelengthLeft(QString frequency);

    /**
     * @brief Emitted after the host successfully closes a specific wavelength via CloseWavelength().
     * @param frequency The frequency identifier of the wavelength that was closed.
     */
    void wavelengthClosed(QString frequency);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit WavelengthLeaver(QObject *parent = nullptr) : QObject(parent) {
    }

    /**
     * @brief Private destructor.
     */
    ~WavelengthLeaver() override = default;

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    WavelengthLeaver(const WavelengthLeaver &) = delete;

    /**
     * @brief Deleted assignment operator to prevent assignment.
     */
    WavelengthLeaver &operator=(const WavelengthLeaver &) = delete;
};

#endif // WAVELENGTH_LEAVER_H
