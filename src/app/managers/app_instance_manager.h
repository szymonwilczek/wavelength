#ifndef APP_INSTANCE_MANAGER_H
#define APP_INSTANCE_MANAGER_H

#include <QObject>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMutex>
#include <QPropertyAnimation>
#include <QTimer>
#include <thread>

class BlobAnimation;
class QMainWindow;

/**
 * @brief Structure holding information about a connected application instance.
 */
struct InstanceInfo {
    QString instance_id; ///< Unique identifier for the instance.
    QPointF blob_center; ///< Center position of the blob within its window.
    QPoint window_position; ///< Global position of the instance's window.
    QSize window_size; ///< Size of the instance's window.
    bool is_creator; ///< Flag indicating if this instance is the creator (was opened first).
};

/**
 * @brief Manages communication and interaction between multiple instances of the application.
 *
 * This class handles the detection of existing instances, establishing a local server/client
 * connection and exchanging position data between instances.
 * It also implements an attraction and absorption mechanism where client instances are drawn
 * towards the creator instance and eventually absorbed (closed).
 */
class AppInstanceManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructs an AppInstanceManager.
     * @param window Pointer to the main application window.
     * @param blob Pointer to the BlobAnimation object associated with this instance.
     * @param parent Optional parent QObject.
     */
    explicit AppInstanceManager(QMainWindow *window, BlobAnimation *blob, QObject *parent = nullptr);

    /**
     * @brief Destructor. Cleans up the resources.
     */
    ~AppInstanceManager() override;

    /**
     * @brief Starts the instance manager.
     *
     * Determines if another instance is running. If so, connect as a client.
     * Otherwise, starts a local server and becomes the creator instance.
     * Initializes timers and the attraction thread.
     * @return True if startup was successful, false otherwise (though currently always returns true).
     */
    bool Start();

    /**
     * @brief Gets the unique identifier for this instance.
     * @return The instance ID as a QString.
     */
    [[nodiscard]] QString GetInstanceId() const { return instance_id_; }

    /**
     * @brief Checks if this instance is the creator (server - was opened first).
     * @return True if this instance is the creator, false otherwise.
     */
    [[nodiscard]] bool IsCreator() const { return is_creator_; }

    /**
     * @brief Statically checks if another instance of the application is already running.
     * @return True if another instance is detected, false otherwise.
     */
    static bool IsAnotherInstanceRunning();

signals:
    /**
     * @brief Emitted when a new client instance connects (on the server) or when connection to the server is established (on the client).
     * @param instance_id The ID of the connected instance.
     */
    void instanceConnected(QString instance_id);

    /**
     * @brief Emitted when a client instance disconnects (on the server) or when the connection to the server is lost (on the client).
     * @param instance_id The ID of the disconnected instance.
     */
    void instanceDisconnected(QString instance_id);

    /**
     * @brief Emitted when position data from another instance is received.
     * @param instance_id The ID of the instance whose position changed.
     * @param blob_center The new center position of the blob in the other instance's window.
     * @param window_position The new global position of the other instance's window.
     */
    void otherInstancePositionHasChanged(QString instance_id, const QPointF &blob_center,
                                         const QPoint &window_position);

private slots:
    /**
     * @brief Slot called when a new client attempts to connect to the server.
     */
    void onNewConnection();

    /**
    * @brief Slot called when a connected socket disconnects. Handles cleanup and potential role change (the client becoming creator).
    */
    void clientDisconnected();

    /**
     * @brief Slot called when data can be read from a connected socket.
     */
    void readData();

    /**
     * @brief Slot called periodically by position_timer_ to send the current instance's position data.
     */
    void sendPositionUpdate();

private:
    /**
     * @brief Defines the types of messages exchanged between instances.
     */
    enum MessageType : quint8 {
        kPositionUpdate = 1, ///< Message containing position and size data.
        kIdentify = 5, ///< Message sent by a client to identify itself to the server.
        kIdentifyResponse = 6, ///< Message sent by the server in response to an Identify message.
    };

    /**
     * @brief Sets up the QLocalServer for the creator instance.
     */
    void SetupServer();

    /**
     * @brief Establishes a connection to the server instance for a client instance.
     */
    void ConnectToServer();

    /**
     * @brief Processes an incoming message received from a socket.
     * @param message The raw byte array containing the message data.
     * @param sender Pointer to the socket that sent the message (used by the server to identify clients). Null for client processing.
     * @return True if the message was processed successfully, false otherwise.
     */
    bool ProcessMessage(const QByteArray &message, QLocalSocket *sender = nullptr);

    /**
     * @brief Sends a message to all connected clients (only applicable for the creator instance).
     * @param message The message to send.
     */
    void SendToAllClients(const QByteArray &message);

    /**
     * @brief Creates a QByteArray message containing the current instance's position data.
     * @return A QByteArray ready to be sent over the socket.
     */
    [[nodiscard]] QByteArray CreatePositionMessage() const;

    /**
     * @brief Initializes and starts the background thread responsible for attraction logic.
     */
    void InitAttractionThread();

    /**
     * @brief Applies a force to the client window, pulling it towards the target position (creator window).
     * @param target_position The global center position of the creator instance's window.
     */
    void ApplyAttractionForce(const QPointF &target_position);

    /**
     * @brief Starts the animation sequence for absorbing a client instance.
     * This involves moving the client window to the creator's position and fading it out.
     */
    void StartAbsorptionAnimation();

    /**
     * @brief Finalizes the absorption process by quitting the client application.
     */
    void FinalizeAbsorption();

    QMainWindow *main_window_; ///< Pointer to the main application window.
    BlobAnimation *blob_; ///< Pointer to the BlobAnimation object.
    QLocalServer *server_ = nullptr; ///< Local server instance (used only by the creator).
    QLocalSocket *socket_ = nullptr; ///< Local socket instance (used only by clients).
    QTimer position_timer_; ///< Timer to periodically send position updates.
    QTimer absorption_check_timer_; ///< Timer potentially used for absorption checks (currently seems unused).
    QMutex instances_mutex_; ///< Mutex to protect access to connected_instances_.

    std::atomic<bool> is_creator_{false}; ///< Atomic flag indicating if this instance is the creator.
    QString instance_id_; ///< Unique identifier for this application instance.

    QVector<InstanceInfo> connected_instances_;
    ///< List of currently known connected instances. Protected by instances_mutex_.
    QHash<QLocalSocket *, QString> client_ids_;
    ///< Maps client sockets to their instance IDs (used only by the creator).
    std::unique_ptr<std::thread> attraction_thread_; ///< Background thread for attraction logic.
    std::atomic<bool> is_thread_running{false}; ///< Atomic flag to control the attraction thread's loop.

    static constexpr int kUpdateIntervalMs = 50; ///< Interval (in ms) for sending position updates.
    static const QString kServerName; ///< Name used for the local server discovery.

    static constexpr double kAttractionForce = 0.5;
    ///< Deprecated attraction force constant (logic now uses kSmoothForce).
    static constexpr double kAbsorptionDistance = 50.0; ///< Distance (in pixels) at which absorption starts.

    std::atomic<bool> is_being_absorbed_ = false;
    ///< Atomic flag indicating if this client instance is currently being absorbed.
};

#endif // APP_INSTANCE_MANAGER_H
