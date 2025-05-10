#include "app_instance_manager.h"
#include "../../blob/core/blob_animation.h"
#include <QMainWindow>
#include <QDataStream>
#include <QCoreApplication>
#include <QUuid>
#include <QScreen>
#include <QThread>
#include <QRandomGenerator>
#include <chrono>
#include <QSequentialAnimationGroup>

const QString AppInstanceManager::kServerName = "pk4-projekt-blob-animation";

AppInstanceManager::AppInstanceManager(QMainWindow *window, BlobAnimation *blob, QObject *parent)
    : QObject(parent),
      main_window_(window),
      blob_(blob),
      instance_id_(QUuid::createUuid().toString()) {
    position_timer_.setInterval(kUpdateIntervalMs);
    connect(&position_timer_, &QTimer::timeout, this, &AppInstanceManager::sendPositionUpdate);
}

AppInstanceManager::~AppInstanceManager() {
    if (attraction_thread_ && is_thread_running) {
        is_thread_running = false;
        attraction_thread_->join();
    }
    if (server_) {
        server_->close();
    }

    if (socket_) {
        socket_->disconnectFromServer();
        socket_->close();
    }
}

bool AppInstanceManager::Start() {
    if (IsAnotherInstanceRunning()) {
        is_creator_ = false;
        ConnectToServer();
    } else {
        is_creator_ = true;
        SetupServer();
    }
    position_timer_.start();
    absorption_check_timer_.start();
    InitAttractionThread();

    if (!is_creator_) {
        main_window_->setWindowFlags(main_window_->windowFlags() | Qt::WindowStaysOnTopHint);
        main_window_->show();
    }

    return true;
}

bool AppInstanceManager::IsAnotherInstanceRunning() {
    QLocalSocket socket;
    socket.connectToServer(kServerName);
    const bool exists = socket.waitForConnected(500);
    if (exists) {
        socket.disconnectFromServer();
    }
    return exists;
}

void AppInstanceManager::onNewConnection() {
    const QLocalSocket *client_socket = server_->nextPendingConnection();

    connect(client_socket, &QLocalSocket::readyRead, this, &AppInstanceManager::readData);
    connect(client_socket, &QLocalSocket::disconnected, this, &AppInstanceManager::clientDisconnected);
}

void AppInstanceManager::clientDisconnected() {
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) return;

    if (is_creator_) {
        if (const QString clientId = client_ids_.value(socket, QString()); !clientId.isNull()) {
            {
                QMutexLocker locker(&instances_mutex_);
                connected_instances_.erase(
                    std::ranges::remove_if(connected_instances_,
                                           [&](const InstanceInfo &info) {
                                               return info.instance_id == clientId;
                                           }).begin(),
                    connected_instances_.end()
                );
            }

            client_ids_.remove(socket);
            emit instanceDisconnected(clientId);

            QMetaObject::invokeMethod(main_window_, [this]() {
                if (main_window_ && !main_window_->isEnabled()) {
                    main_window_->setEnabled(true);
                }
                if (main_window_ && !main_window_->isActiveWindow()) {
                    main_window_->activateWindow();
                    main_window_->raise();
                }
                if (main_window_ && main_window_->testAttribute(Qt::WA_TransparentForMouseEvents)) {
                    main_window_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
                }
            }, Qt::QueuedConnection);
        } else {
        }
    } else {
        if (!IsAnotherInstanceRunning()) {
            is_creator_ = true;
            if (attraction_thread_ && is_thread_running) {
                is_thread_running = false;
                if (attraction_thread_->joinable()) {
                    attraction_thread_->join();
                }
                attraction_thread_.reset();
            }
            if (socket_) {
                socket_->deleteLater();
                socket_ = nullptr;
            } {
                QMutexLocker locker(&instances_mutex_);
                connected_instances_.clear();
            }
            SetupServer();
        }
    }

    socket->deleteLater();
}

void AppInstanceManager::readData() {
    const auto socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) return;

    while (socket->bytesAvailable() > 0) {
        QByteArray data = socket->readAll();
        ProcessMessage(data, socket);
    }
}

void AppInstanceManager::sendPositionUpdate() {
    const QByteArray message = CreatePositionMessage();

    if (is_creator_) {
        SendToAllClients(message);
    } else if (socket_ && socket_->isOpen()) {
        socket_->write(message);
    }
}

void AppInstanceManager::SetupServer() {
    server_ = new QLocalServer(this);
    QLocalServer::removeServer(kServerName);

    if (!server_->listen(kServerName)) {
        qWarning() << "[INSTANCE MANAGER] SERVER CANNOT BE STARTED:" << server_->errorString();
        return;
    }

    connect(server_, &QLocalServer::newConnection, this, &AppInstanceManager::onNewConnection);
}

void AppInstanceManager::ConnectToServer() {
    socket_ = new QLocalSocket(this);

    connect(socket_, &QLocalSocket::readyRead, this, &AppInstanceManager::readData);
    connect(socket_, &QLocalSocket::disconnected, this, &AppInstanceManager::clientDisconnected);

    socket_->connectToServer(kServerName);

    if (socket_->waitForConnected(1000)) {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << static_cast<quint8>(kIdentify) << instance_id_;
        socket_->write(message);
    } else {
        qWarning() << "[INSTANCE MANAGER] CANNOT ESTABLISH CONNECTION" << socket_->errorString();
    }
}

bool AppInstanceManager::ProcessMessage(const QByteArray &message, QLocalSocket *sender) {
    QDataStream stream(message);
    quint8 message_type;
    stream >> message_type;

    switch (message_type) {
        case kPositionUpdate: {
            QString id;
            QPointF blob_center;
            QPoint window_position;
            QSize window_size;

            stream >> id >> blob_center >> window_position >> window_size;

            QMutexLocker locker(&instances_mutex_);
            bool found = false;
            for (auto &instance: connected_instances_) {
                if (instance.instance_id == id) {
                    instance.blob_center = blob_center;
                    instance.window_position = window_position;
                    instance.window_size = window_size;
                    found = true;
                    break;
                }
            }

            if (!found) {
                InstanceInfo info;
                info.instance_id = id;
                info.blob_center = blob_center;
                info.window_position = window_position;
                info.window_size = window_size;
                info.is_creator = is_creator_ ? false : (sender == nullptr);
                connected_instances_.append(info);

                if (sender && is_creator_) {
                    client_ids_[sender] = id;
                    emit instanceConnected(id);
                }
            }

            emit otherInstancePositionHasChanged(id, blob_center, window_position);
            return true;
        }

        case kIdentify: {
            QString id;
            stream >> id;

            if (is_creator_ && sender) {
                client_ids_[sender] = id;
                QByteArray response;
                QDataStream response_stream(&response, QIODevice::WriteOnly);
                response_stream << static_cast<quint8>(kIdentifyResponse) << instance_id_ << true;
                sender->write(response);
                sender->write(CreatePositionMessage());

                emit instanceConnected(id);
            }
            return true;
        }

        case kIdentifyResponse: {
            QString id;
            bool is_creator;
            stream >> id >> is_creator;


            if (!is_creator_) {
                InstanceInfo info;
                info.instance_id = id;
                info.is_creator = true;

                QMutexLocker locker(&instances_mutex_);
                connected_instances_.append(info);

                emit instanceConnected(id);
            }
            return true;
        }

        default:
            qWarning() << "[INSTANCE MANAGER] UNKNOWN TYPE OF A MESSAGE: " << message_type;
            return false;
    }
}

void AppInstanceManager::SendToAllClients(const QByteArray &message) {
    if (!is_creator_) return;

    for (auto it = client_ids_.begin(); it != client_ids_.end(); ++it) {
        if (QLocalSocket *socket = it.key(); socket && socket->isOpen()) {
            socket->write(message);
        }
    }
}

QByteArray AppInstanceManager::CreatePositionMessage() const {
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);

    const QPointF blobCenter = blob_->GetBlobCenter();
    const QPoint windowPos = main_window_->pos();
    const QSize windowSize = main_window_->size();

    stream << static_cast<quint8>(kPositionUpdate) << instance_id_ << blobCenter << windowPos << windowSize;

    return message;
}

void AppInstanceManager::InitAttractionThread() {
    is_thread_running = true;
    attraction_thread_ = std::make_unique<std::thread>([this]() {
        while (is_thread_running) {
            constexpr int sleep_time_ms = 16;
            if (is_being_absorbed_ || is_absorbing_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
                continue;
            }

            QVector<InstanceInfo> instances_copy; {
                QMutexLocker locker(&instances_mutex_);
                instances_copy = connected_instances_;
            }

            if (!is_creator_) {
                for (const auto &instance: instances_copy) {
                    if (instance.is_creator) {
                        QPointF blob_center = blob_->GetBlobCenter();
                        QPoint window_position = main_window_->pos();
                        QPointF global_position(window_position.x() + blob_center.x(),
                                                window_position.y() + blob_center.y());

                        QPointF other_blob_center = instance.blob_center;
                        QPoint other_window_position = instance.window_position;
                        QPointF global_creator_position(other_window_position.x() + other_blob_center.x(),
                                                        other_window_position.y() + other_blob_center.y());

                        if (const double distance = QLineF(global_position, global_creator_position).length();
                            distance < kAbsorptionDistance && !is_being_absorbed_) {
                            QMetaObject::invokeMethod(this, [this]() {
                                is_being_absorbed_ = true;
                                main_window_->setWindowFlags(main_window_->windowFlags() | Qt::WindowStaysOnTopHint);
                                main_window_->show();
                                StartAbsorptionAnimation();
                            }, Qt::QueuedConnection);
                        } else {
                            QMetaObject::invokeMethod(this, [this, global_creator_position]() {
                                ApplyAttractionForce(global_creator_position);
                            }, Qt::QueuedConnection);
                        }
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
        }
    });
}

void AppInstanceManager::ApplyAttractionForce(const QPointF &target_position) {
    if (is_being_absorbed_ || is_absorbing_) return;

    const QPoint current_position = main_window_->pos();
    const QSize window_size = main_window_->size();

    const QPointF current_center(
        current_position.x() + window_size.width() / 2.0,
        current_position.y() + window_size.height() / 2.0
    );

    const QPointF difference = target_position - current_center;

    static constexpr double kSmoothForce = 0.02;

    const QPointF new_center = current_center + (difference * kSmoothForce);

    const QPointF new_position(
        new_center.x() - window_size.width() / 2.0,
        new_center.y() - window_size.height() / 2.0
    );

    main_window_->setEnabled(false);
    main_window_->move(new_position.toPoint());

    if (difference.manhattanLength() < kAbsorptionDistance && !is_being_absorbed_) {
        is_being_absorbed_ = true;
        StartAbsorptionAnimation();
    }
}

void AppInstanceManager::StartAbsorptionAnimation() {
    const auto absorption_sequence = new QSequentialAnimationGroup(this);

    // 1. position alignment
    auto *position_animation = new QPropertyAnimation(main_window_, "pos");
    position_animation->setDuration(500);
    position_animation->setEasingCurve(QEasingCurve::InOutQuad);

    QPoint target_position; {
        QMutexLocker locker(&instances_mutex_);
        for (const auto &instance: connected_instances_) {
            if (instance.is_creator) {
                target_position = instance.window_position;
                break;
            }
        }
    }

    position_animation->setStartValue(main_window_->pos());
    position_animation->setEndValue(target_position);
    absorption_sequence->addAnimation(position_animation);

    // 2. opacity animation
    auto *fade_animation = new QPropertyAnimation(main_window_, "windowOpacity");
    fade_animation->setDuration(2000);
    fade_animation->setEasingCurve(QEasingCurve::InOutCubic);

    fade_animation->setKeyValueAt(0.0, 1.0); // start: full visibility
    fade_animation->setKeyValueAt(0.2, 0.95); // subtle start of fading
    fade_animation->setKeyValueAt(0.4, 0.8); // gradual disappearance
    fade_animation->setKeyValueAt(0.6, 0.6); // half-visible
    fade_animation->setKeyValueAt(0.8, 0.3); // more than half-visible
    fade_animation->setKeyValueAt(0.9, 0.15); // almost invisible
    fade_animation->setKeyValueAt(1.0, 0.0); // fully invisible

    absorption_sequence->addAnimation(fade_animation);

    connect(absorption_sequence, &QSequentialAnimationGroup::finished, this, [this]() {
        FinalizeAbsorption();
    });

    absorption_sequence->start(QAbstractAnimation::DeleteWhenStopped);
}

void AppInstanceManager::FinalizeAbsorption() {
    if (is_being_absorbed_) {
        QMetaObject::invokeMethod(this, [this]() {
            QTimer::singleShot(100, [this]() {
                qApp->quit();
            });
        }, Qt::QueuedConnection);
    }
}
