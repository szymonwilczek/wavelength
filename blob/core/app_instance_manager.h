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
    void instanceConnected(QString instanceId);
    void instanceDisconnected(QString instanceId);

public slots:
    void updateBlobPosition();

private slots:
    void onNewConnection();
    void readData();
    void clientDisconnected();
    void sendPositionUpdate();

private:
    enum MessageType {
        POSITION_UPDATE = 1,
        IDENTIFY = 5,
        IDENTITY_RESPONSE = 6,
    };

    void setupServer();
    void connectToServer();
    bool processMessage(const QByteArray& message, QLocalSocket* sender = nullptr);
    void sendToAllClients(const QByteArray& message);
    void sendToCreator(const QByteArray& message);
    QByteArray createPositionMessage() const;
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

    static constexpr int UPDATE_INTERVAL_MS = 50;
    static const QString SERVER_NAME;
};

#endif // APP_INSTANCE_MANAGER_H