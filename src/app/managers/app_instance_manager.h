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
#include <QPropertyAnimation>
#include <QSizeF>
#include <thread>

class BlobAnimation;
class QMainWindow;

struct InstanceInfo {
    QString instance_id;
    QPointF blob_center;
    QPoint window_position;
    QSize window_size;
    bool is_creator;
};

class AppInstanceManager final : public QObject {
    Q_OBJECT

public:
    explicit AppInstanceManager(QMainWindow* window, BlobAnimation* blob, QObject* parent = nullptr);
    ~AppInstanceManager() override;

    bool IsCreator() const { return is_creator_; }
    QString GetInstanceId() const { return instance_id_; }
    bool Start();

    static bool IsAnotherInstanceRunning();

signals:
    void otherInstancePositionHasChanged(QString instanceId, const QPointF& blobCenter, const QPoint& windowPos);
    void instanceConnected(QString instanceId);
    void instanceDisconnected(QString instanceId);


private slots:
    void onNewConnection();
    void readData();
    void clientDisconnected();
    void sendPositionUpdate();

private:
    enum MessageType {
        kPositionUpdate = 1,
        kIdentify = 5,
        kIdentifyResponse = 6,
    };

    void SetupServer();
    void ConnectToServer();
    bool ProcessMessage(const QByteArray& message, QLocalSocket* sender = nullptr);
    void SendToAllClients(const QByteArray& message);
    QByteArray CreatePositionMessage() const;
    void InitAttractionThread();

    QMainWindow* main_window_;
    BlobAnimation* blob_;
    QLocalServer* server_ = nullptr;
    QLocalSocket* socket_ = nullptr;
    QTimer position_timer_;
    QTimer absorption_check_timer_;
    QMutex instances_mutex_;

    std::atomic<bool> is_creator_{false};
    QString instance_id_;

    QVector<InstanceInfo> connected_instances_;
    QHash<QLocalSocket*, QString> client_ids_;
    std::unique_ptr<std::thread> attraction_thread_;
    std::atomic<bool> is_thread_running{false};

    static constexpr int kUpdateIntervalMs = 50;
    static const QString kServerName;

    static constexpr double kAttractionForce = 0.5;
    static constexpr double kAbsorptionDistance = 50.0;

    void ApplyAttractionForce(const QPointF& target_position);
    void StartAbsorptionAnimation();
    void FinalizeAbsorption();

    QPropertyAnimation* animation_window_ = nullptr;
    bool is_being_absorbed_ = false;
    bool is_absorbing_ = false;
    QTimer absorption_timer_;
};

#endif // APP_INSTANCE_MANAGER_H