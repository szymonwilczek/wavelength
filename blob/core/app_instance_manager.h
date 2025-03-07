#ifndef APP_INSTANCE_MANAGER_H
#define APP_INSTANCE_MANAGER_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QMutex>
#include <QVector>
#include <memory>
#include <atomic>
#include <QSizeF>
#include <thread>

class BlobAnimation;
class QMainWindow;

struct InstanceInfo {
    QString instanceId;
    QPointF blobCenter;
    QPoint windowPosition;
    QSize windowSize;
    bool isCreator;
};

class AppInstanceManager : public QObject {
    Q_OBJECT

public:
    explicit AppInstanceManager(QMainWindow* window, BlobAnimation* blob, QObject* parent = nullptr);
    ~AppInstanceManager();

    bool isCreator() const { return m_isCreator; }
    QString getInstanceId() const { return m_instanceId; }
    bool start();

    static bool isAnotherInstanceRunning();

signals:
    void otherInstancePositionChanged(QString instanceId, const QPointF& blobCenter, const QPoint& windowPos);
    void instanceAbsorbed(QString instanceId);
    void absorptionStarted(QString instanceId);
    void absorptionCancelled(QString instanceId);
    void instanceConnected(QString instanceId);
    void instanceDisconnected(QString instanceId);

public slots:
    void updateBlobPosition();
    void checkForAbsorption();

private slots:
    void onNewConnection();
    void readData();
    void clientDisconnected();
    void sendPositionUpdate();

private:
    enum MessageType {
        POSITION_UPDATE = 1,
        ABSORPTION_START = 2,
        ABSORPTION_COMPLETE = 3,
        ABSORPTION_CANCELLED = 4,
        IDENTIFY = 5,
        IDENTITY_RESPONSE = 6,
        ABSORPTION_PROGRESS = 7
    };

    void setupServer();
    void connectToServer();
    bool processMessage(const QByteArray& message, QLocalSocket* sender = nullptr);
    void sendToAllClients(const QByteArray& message);
    void sendToCreator(const QByteArray& message);
    QByteArray createPositionMessage() const;
    QByteArray createAbsorptionMessage(MessageType type, QString targetId) const;

    bool isInAbsorptionRange(const QPointF& otherBlobCenter, const QPoint& otherWindowPos) const;

    void initAttractionThread();

private:
    QMainWindow* m_window;
    BlobAnimation* m_blob;
    QLocalServer* m_server = nullptr;
    QLocalSocket* m_socket = nullptr;
    QTimer m_positionTimer;
    QTimer m_absorptionCheckTimer;
    QMutex m_instancesMutex;

    std::atomic<bool> m_isCreator{false};
    QString m_instanceId;

    QVector<InstanceInfo> m_connectedInstances;
    QHash<QLocalSocket*, QString> m_clientIds;
    std::unique_ptr<std::thread> m_attractionThread;
    std::atomic<bool> m_threadRunning{false};
    struct AbsorptionState {
        bool inProgress = false;
        QString targetId;
        float progress = 0.0f;
    };

    AbsorptionState m_absorption;
    static constexpr int ABSORPTION_RANGE = 150;
    static constexpr int UPDATE_INTERVAL_MS = 50;
    static constexpr int ABSORPTION_CHECK_INTERVAL_MS = 100;
    static const QString SERVER_NAME;
};

#endif // APP_INSTANCE_MANAGER_H