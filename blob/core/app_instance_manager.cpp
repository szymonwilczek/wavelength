#include "app_instance_manager.h"
#include "blob_animation.h"
#include <QMainWindow>
#include <QDataStream>
#include <QCoreApplication>
#include <QUuid>
#include <QScreen>
#include <QBuffer>
#include <QThread>
#include <QDebug>
#include <QTime>
#include <chrono>
#include <QDateTime>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QRandomGenerator>
#endif


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
    bool exists = socket.waitForConnected(500);
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

    return true;
}

void AppInstanceManager::setupServer() {
    qDebug() << "Tworzenie serwera instancji (Creator)";
    m_server = new QLocalServer(this);
    QLocalServer::removeServer(SERVER_NAME);

    if (!m_server->listen(SERVER_NAME)) {
        qWarning() << "Nie można uruchomić serwera:" << m_server->errorString();
        return;
    }

    connect(m_server, &QLocalServer::newConnection, this, &AppInstanceManager::onNewConnection);
    qDebug() << "Serwer uruchomiony, instancja jest Creatorem";
}

void AppInstanceManager::connectToServer() {
    qDebug() << "Łączenie z istniejącą instancją jako klient";
    m_socket = new QLocalSocket(this);

    connect(m_socket, &QLocalSocket::readyRead, this, &AppInstanceManager::readData);
    connect(m_socket, &QLocalSocket::disconnected, this, &AppInstanceManager::clientDisconnected);

    m_socket->connectToServer(SERVER_NAME);

    if (m_socket->waitForConnected(1000)) {
        qDebug() << "Połączono z serwerem";
        QByteArray message;
        QDataStream stream(&message, QIODevice::WriteOnly);
        stream << quint8(IDENTIFY) << m_instanceId;
        m_socket->write(message);
    } else {
        qWarning() << "Nie udało się połączyć z serwerem:" << m_socket->errorString();
    }
}

void AppInstanceManager::onNewConnection() {
    QLocalSocket* clientSocket = m_server->nextPendingConnection();

    connect(clientSocket, &QLocalSocket::readyRead, this, &AppInstanceManager::readData);
    connect(clientSocket, &QLocalSocket::disconnected, this, &AppInstanceManager::clientDisconnected);

    qDebug() << "Nowe połączenie klienta";
}

void AppInstanceManager::readData() {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    while (socket->bytesAvailable() > 0) {
        QByteArray data = socket->readAll();
        processMessage(data, socket);
    }
}

void AppInstanceManager::clientDisconnected() {
    QLocalSocket* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    if (m_isCreator) {
        QString clientId = m_clientIds.value(socket, QString(-1));
        if (clientId != -1) {
            qDebug() << "Klient rozłączony:" << clientId;

            QMutexLocker locker(&m_instancesMutex);
            for (int i = 0; i < m_connectedInstances.size(); i++) {
                if (m_connectedInstances[i].instanceId == clientId) {
                    m_connectedInstances.removeAt(i);
                    break;
                }
            }

            m_clientIds.remove(socket);
            emit instanceDisconnected(clientId);
        }
    } else {
        qDebug() << "Rozłączono z serwerem";
        if (!isAnotherInstanceRunning()) {
            m_isCreator = true;
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
                    qDebug() << "Dodano nową instancję:" << id;
                }
            }

            emit otherInstancePositionChanged(id, blobCenter, windowPos);
            return true;
        }

        case IDENTIFY: {
            QString id;
            stream >> id;

            if (m_isCreator && sender) {
                qDebug() << "Zidentyfikowano nowego klienta:" << id;
                m_clientIds[sender] = id;
                QByteArray response;
                QDataStream responseStream(&response, QIODevice::WriteOnly);
                responseStream << quint8(IDENTITY_RESPONSE) << m_instanceId << true;
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
            qWarning() << "Nieznany typ wiadomości:" << messageType;
            return false;
    }
}

void AppInstanceManager::sendPositionUpdate() {
    updateBlobPosition();

    QByteArray message = createPositionMessage();

    if (m_isCreator) {
        sendToAllClients(message);
    } else if (m_socket && m_socket->isOpen()) {
        m_socket->write(message);
    }
}

QByteArray AppInstanceManager::createPositionMessage() const {
    QByteArray message;
    QDataStream stream(&message, QIODevice::WriteOnly);

    QPointF blobCenter = m_blob->getBlobCenter();
    QPoint windowPos = m_window->pos();
    QSize windowSize = m_window->size();



    stream << quint8(POSITION_UPDATE) << m_instanceId << blobCenter << windowPos << windowSize;

    return message;
}

void AppInstanceManager::updateBlobPosition() {
}

void AppInstanceManager::sendToAllClients(const QByteArray& message) {
    if (!m_isCreator) return;

    for (auto it = m_clientIds.begin(); it != m_clientIds.end(); ++it) {
        QLocalSocket* socket = it.key();
        if (socket && socket->isOpen()) {
            socket->write(message);
        }
    }
}

void AppInstanceManager::sendToCreator(const QByteArray& message) {
    if (m_isCreator) return;

    if (m_socket && m_socket->isOpen()) {
        m_socket->write(message);
    }
}


void AppInstanceManager::initAttractionThread() {
    m_threadRunning = true;
    m_attractionThread = std::make_unique<std::thread>([this]() {
        const int sleepTimeMs = 100;

        while (m_threadRunning) {
            QVector<InstanceInfo> instancesCopy;
            {
                QMutexLocker locker(&m_instancesMutex);
                instancesCopy = m_connectedInstances;
            }
            if (m_isCreator) {
                for (const auto& instance : instancesCopy) {
                    if (instance.instanceId == m_instanceId || instance.isCreator) {
                        continue;
                    }

                    QPointF myBlobCenter = m_blob->getBlobCenter();
                    QPoint myWindowPos = m_window->pos();
                    QPointF globalMyPos(myWindowPos.x() + myBlobCenter.x(), myWindowPos.y() + myBlobCenter.y());

                    QPointF otherBlobCenter = instance.blobCenter;
                    QPoint otherWindowPos = instance.windowPosition;
                    QPointF globalOtherPos(otherWindowPos.x() + otherBlobCenter.x(), otherWindowPos.y() + otherBlobCenter.y());

                    QPointF diff = globalMyPos - globalOtherPos;
                    double distance = sqrt(diff.x() * diff.x() + diff.y() * diff.y());

                    if (distance > 10.0) {

                    }
                }
            } else {
                for (const auto& instance : instancesCopy) {
                    if (instance.isCreator) {
                        QPointF myBlobCenter = m_blob->getBlobCenter();
                        QPoint myWindowPos = m_window->pos();
                        QPointF globalMyPos(myWindowPos.x() + myBlobCenter.x(), myWindowPos.y() + myBlobCenter.y());
                        if (instance.blobCenter.isNull()) {
                            continue;
                        }

                        QPointF otherBlobCenter = instance.blobCenter;
                        QPoint otherWindowPos = instance.windowPosition;
                        QPointF globalCreatorPos(otherWindowPos.x() + otherBlobCenter.x(), otherWindowPos.y() + otherBlobCenter.y());

                        QPointF diff = globalCreatorPos - globalMyPos;
                        double distance = sqrt(diff.x() * diff.x() + diff.y() * diff.y());


                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        }
    });
}