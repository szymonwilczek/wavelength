#include "app_instance_manager.h"
#include "../../blob/core/blob_animation.h"
#include <QMainWindow>
#include <QDataStream>
#include <QCoreApplication>
#include <QUuid>
#include <QScreen>
#include <QThread>
#include <QDebug>
#include <QRandomGenerator>
#include <chrono>
#include <QSequentialAnimationGroup>

// ReSharper disable once CppDFATimeOver
const QString AppInstanceManager::SERVER_NAME = "pk4-projekt-blob-animation";

AppInstanceManager::AppInstanceManager(QMainWindow* window, BlobAnimation* blob, QObject* parent)
    : QObject(parent),
      m_window(window),
      m_blob(blob),
      m_instanceId(QUuid::createUuid().toString())
{
    m_positionTimer.setInterval(UPDATE_INTERVAL_MS);
    connect(&m_positionTimer, &QTimer::timeout, this, &AppInstanceManager::sendPositionUpdate);
}

AppInstanceManager::~AppInstanceManager() {
    if (m_attractionThread && m_threadRunning) {
        m_threadRunning = false;
        m_attractionThread->join();
    }
    if (m_server) {
        m_server->close();
    }

    if (m_socket) {
        m_socket->disconnectFromServer();
        m_socket->close();
    }
}

bool AppInstanceManager::isAnotherInstanceRunning() {
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME);
    const bool exists = socket.waitForConnected(500);
    if (exists) {
        socket.disconnectFromServer();
    }
    return exists;
}

bool AppInstanceManager::start() {
    if (isAnotherInstanceRunning()) {
        m_isCreator = false;
        connectToServer();
    } else {
        m_isCreator = true;
        setupServer();
    }
    m_positionTimer.start();
    m_absorptionCheckTimer.start();
    initAttractionThread();

    if (!m_isCreator) {
        m_window->setWindowFlags(m_window->windowFlags() | Qt::WindowStaysOnTopHint);
        m_window->show();
    }

    return true;
}

void AppInstanceManager::setupServer() {
    m_server = new QLocalServer(this);
    QLocalServer::removeServer(SERVER_NAME);

    if (!m_server->listen(SERVER_NAME)) {
        qWarning() << "[INSTANCE MANAGER] SERVER CANNOT BE STARTED:" << m_server->errorString();
        return;
    }

    connect(m_server, &QLocalServer::newConnection, this, &AppInstanceManager::onNewConnection);
}

void AppInstanceManager::connectToServer() {
    m_socket = new QLocalSocket(this);

    connect(m_socket, &QLocalSocket::readyRead, this, &AppInstanceManager::readData);
    connect(m_socket, &QLocalSocket::disconnected, this, &AppInstanceManager::clientDisconnected);

    m_socket->connectToServer(SERVER_NAME);

    if (m_socket->waitForConnected(1000)) {
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << static_cast<quint8>(IDENTIFY) << m_instanceId;
        m_socket->write(message);
    } else {
        qWarning() << "[INSTANCE MANAGER] CANNOT ESTABLISH CONNECTION" << m_socket->errorString();
    }
}

void AppInstanceManager::onNewConnection() {
    QLocalSocket* clientSocket = m_server->nextPendingConnection();

    connect(clientSocket, &QLocalSocket::readyRead, this, &AppInstanceManager::readData);
    connect(clientSocket, &QLocalSocket::disconnected, this, &AppInstanceManager::clientDisconnected);
}

void AppInstanceManager::readData() {
    const auto socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    while (socket->bytesAvailable() > 0) {
        QByteArray data = socket->readAll();
        processMessage(data, socket);
    }
}

void AppInstanceManager::clientDisconnected() {
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    if (m_isCreator) {
        if (const QString clientId = m_clientIds.value(socket, QString()); !clientId.isNull()) {
            qDebug() << "[INSTANCE MANAGER] Client disconnected:" << clientId;
            {
                QMutexLocker locker(&m_instancesMutex);
                m_connectedInstances.erase(
                    std::remove_if(m_connectedInstances.begin(), m_connectedInstances.end(),
                                   [&](const InstanceInfo& info){ return info.instanceId == clientId; }),
                    m_connectedInstances.end()
                );
            }

            m_clientIds.remove(socket);
            emit instanceDisconnected(clientId);

            QMetaObject::invokeMethod(m_window, [this]() {
                if (m_window && !m_window->isEnabled()) {
                    qDebug() << "[INSTANCE MANAGER] Re-enabling main window after client disconnect.";
                    m_window->setEnabled(true);
                }
                if (m_window && !m_window->isActiveWindow()) {
                     qDebug() << "[INSTANCE MANAGER] Activating main window after client disconnect.";
                    m_window->activateWindow();
                    m_window->raise();
                }
                 if (m_window && m_window->testAttribute(Qt::WA_TransparentForMouseEvents)) {
                    qDebug() << "[INSTANCE MANAGER] Disabling WA_TransparentForMouseEvents on main window.";
                    m_window->setAttribute(Qt::WA_TransparentForMouseEvents, false);
                 }

            }, Qt::QueuedConnection);

        } else {
             qDebug() << "[INSTANCE MANAGER] Client disconnected, but ID not found for socket.";
        }
    } else {
        qDebug() << "[INSTANCE MANAGER] Disconnected from server.";
        if (!isAnotherInstanceRunning()) {
            qDebug() << "[INSTANCE MANAGER] Server not running, becoming creator.";
            m_isCreator = true;
            if (m_attractionThread && m_threadRunning) {
                m_threadRunning = false;
                if (m_attractionThread->joinable()) {
                    m_attractionThread->join();
                }
                m_attractionThread.reset();
            }
            if(m_socket) {
                m_socket->deleteLater();
                m_socket = nullptr;
            }
            {
                QMutexLocker locker(&m_instancesMutex);
                m_connectedInstances.clear();
            }
            setupServer();
        }
    }

    socket->deleteLater();
}

bool AppInstanceManager::processMessage(const QByteArray& message, QLocalSocket* sender) {
    QDataStream stream(message);
    quint8 messageType;
    stream >> messageType;

    switch (messageType) {
        case POSITION_UPDATE: {
            QString id;
            QPointF blobCenter;
            QPoint windowPos;
            QSize windowSize;

            stream >> id >> blobCenter >> windowPos >> windowSize;

            QMutexLocker locker(&m_instancesMutex);
            bool found = false;
            for (auto& instance : m_connectedInstances) {
                if (instance.instanceId == id) {
                    instance.blobCenter = blobCenter;
                    instance.windowPosition = windowPos;
                    instance.windowSize = windowSize;
                    found = true;
                    break;
                }
            }

            if (!found) {
                InstanceInfo info;
                info.instanceId = id;
                info.blobCenter = blobCenter;
                info.windowPosition = windowPos;
                info.windowSize = windowSize;
                info.isCreator = m_isCreator ? false : (sender == nullptr);
                m_connectedInstances.append(info);

                if (sender && m_isCreator) {
                    m_clientIds[sender] = id;
                    emit instanceConnected(id);
                }
            }

            emit otherInstancePositionChanged(id, blobCenter, windowPos);
            return true;
        }

        case IDENTIFY: {
            QString id;
            stream >> id;

            if (m_isCreator && sender) {
                m_clientIds[sender] = id;
                QByteArray response;
                QDataStream responseStream(&response, QIODevice::WriteOnly);
                responseStream << static_cast<quint8>(IDENTITY_RESPONSE) << m_instanceId << true;
                sender->write(response);
                sender->write(createPositionMessage());

                emit instanceConnected(id);
            }
            return true;
        }

        case IDENTITY_RESPONSE: {
            QString id;
            bool isCreator;
            stream >> id >> isCreator;


            if (!m_isCreator) {
                InstanceInfo info;
                info.instanceId = id;
                info.isCreator = true;

                QMutexLocker locker(&m_instancesMutex);
                m_connectedInstances.append(info);

                emit instanceConnected(id);
            }
            return true;
        }

        default:
            qWarning() << "[INSTANCE MANAGER] UNKNOWN TYPE OF A MESSAGE: " << messageType;
            return false;
    }
}

void AppInstanceManager::sendPositionUpdate() {

    const QByteArray message = createPositionMessage();

    if (m_isCreator) {
        sendToAllClients(message);
    } else if (m_socket && m_socket->isOpen()) {
        m_socket->write(message);
    }
}

QByteArray AppInstanceManager::createPositionMessage() const {
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);

    const QPointF blobCenter = m_blob->getBlobCenter();
    const QPoint windowPos = m_window->pos();
    const QSize windowSize = m_window->size();

    stream << static_cast<quint8>(POSITION_UPDATE) << m_instanceId << blobCenter << windowPos << windowSize;

    return message;
}


void AppInstanceManager::sendToAllClients(const QByteArray& message) {
    if (!m_isCreator) return;

    for (auto it = m_clientIds.begin(); it != m_clientIds.end(); ++it) {
        if (QLocalSocket* socket = it.key(); socket && socket->isOpen()) {
            socket->write(message);
        }
    }
}


void AppInstanceManager::initAttractionThread() {
    m_threadRunning = true;
    m_attractionThread = std::make_unique<std::thread>([this]() {
        while (m_threadRunning) {
            constexpr int sleepTimeMs = 16;
            if (m_isBeingAbsorbed || m_isAbsorbing) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
                continue;
            }

            QVector<InstanceInfo> instancesCopy;
            {
                QMutexLocker locker(&m_instancesMutex);
                instancesCopy = m_connectedInstances;
            }

            if (!m_isCreator) {
                for (const auto& instance : instancesCopy) {
                    if (instance.isCreator) {
                        QPointF myBlobCenter = m_blob->getBlobCenter();
                        QPoint myWindowPos = m_window->pos();
                        QPointF globalMyPos(myWindowPos.x() + myBlobCenter.x(),
                                          myWindowPos.y() + myBlobCenter.y());

                        QPointF otherBlobCenter = instance.blobCenter;
                        QPoint otherWindowPos = instance.windowPosition;
                        QPointF globalCreatorPos(otherWindowPos.x() + otherBlobCenter.x(),
                                               otherWindowPos.y() + otherBlobCenter.y());

                        if (const double distance = QLineF(globalMyPos, globalCreatorPos).length(); distance < ABSORPTION_DISTANCE && !m_isBeingAbsorbed) {
                            QMetaObject::invokeMethod(this, [this]() {
                                m_isBeingAbsorbed = true;
                                m_window->setWindowFlags(m_window->windowFlags() | Qt::WindowStaysOnTopHint);
                                m_window->show();
                                startAbsorptionAnimation();
                            }, Qt::QueuedConnection);
                        } else {
                            QMetaObject::invokeMethod(this, [this, globalCreatorPos]() {
                                applyAttractionForce(globalCreatorPos);
                            }, Qt::QueuedConnection);
                        }
                        break;
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        }
    });
}

void AppInstanceManager::applyAttractionForce(const QPointF& targetPos) {
    if (m_isBeingAbsorbed || m_isAbsorbing) return;

    const QPoint currentPos = m_window->pos();
    const QSize windowSize = m_window->size();

    const QPointF currentCenter(
        currentPos.x() + windowSize.width() / 2.0,
        currentPos.y() + windowSize.height() / 2.0
    );

    const QPointF diff = targetPos - currentCenter;

    static constexpr double SMOOTH_FORCE = 0.02;

    const QPointF newCenter = currentCenter + (diff * SMOOTH_FORCE);

    const QPointF newPos(
        newCenter.x() - windowSize.width() / 2.0,
        newCenter.y() - windowSize.height() / 2.0
    );

    m_window->setEnabled(false);
    m_window->move(newPos.toPoint());

    if (diff.manhattanLength() < ABSORPTION_DISTANCE && !m_isBeingAbsorbed) {
        m_isBeingAbsorbed = true;
        startAbsorptionAnimation();
    }
}

void AppInstanceManager::startAbsorptionAnimation() {
    const auto absorptionSequence = new QSequentialAnimationGroup(this);

    // 1. wyrównanie pozycji
    auto* positionAnimation = new QPropertyAnimation(m_window, "pos");
    positionAnimation->setDuration(500);
    positionAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    QPoint targetPos;
    {
        QMutexLocker locker(&m_instancesMutex);
        for (const auto& instance : m_connectedInstances) {
            if (instance.isCreator) {
                targetPos = instance.windowPosition;
                break;
            }
        }
    }

    positionAnimation->setStartValue(m_window->pos());
    positionAnimation->setEndValue(targetPos);
    absorptionSequence->addAnimation(positionAnimation);

    // 2. animacja przezroczystości
    auto* fadeAnimation = new QPropertyAnimation(m_window, "windowOpacity");
    fadeAnimation->setDuration(2000);
    fadeAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    fadeAnimation->setKeyValueAt(0.0, 1.0);    // Start - pełna widoczność
    fadeAnimation->setKeyValueAt(0.2, 0.95);   // Subtelne rozpoczęcie zanikania
    fadeAnimation->setKeyValueAt(0.4, 0.8);    // Stopniowe zanikanie
    fadeAnimation->setKeyValueAt(0.6, 0.6);    // Połowa przezroczystości
    fadeAnimation->setKeyValueAt(0.8, 0.3);    // Większa przezroczystość
    fadeAnimation->setKeyValueAt(0.9, 0.15);   // Prawie niewidoczne
    fadeAnimation->setKeyValueAt(1.0, 0.0);    // Całkowicie niewidoczne

    absorptionSequence->addAnimation(fadeAnimation);

    connect(absorptionSequence, &QSequentialAnimationGroup::finished, this, [this]() {
        finalizeAbsorption();
    });

    absorptionSequence->start(QAbstractAnimation::DeleteWhenStopped);
}

void AppInstanceManager::finalizeAbsorption() {
    if (m_isBeingAbsorbed) {
        QMetaObject::invokeMethod(this, [this]() {
            QTimer::singleShot(100, [this]() {
                qApp->quit();
            });
        }, Qt::QueuedConnection);
    }
}