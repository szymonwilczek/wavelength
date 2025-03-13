#ifndef WAVELENGTH_MESSAGE_SERVICE_H
#define WAVELENGTH_MESSAGE_SERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QString>
#include <QDebug>

#include "../registry/wavelength_registry.h"
#include "../messages/handler/message_handler.h"
#include "../auth/authentication_manager.h"

class WavelengthMessageService : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageService* getInstance() {
        static WavelengthMessageService instance;
        return &instance;
    }

    bool sendMessage(const QString& message) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        double freq = registry->getActiveWavelength();

        if (freq == -1) {
            qDebug() << "Cannot send message - no active wavelength";
            return false;
        }

        if (!registry->hasWavelength(freq)) {
            qDebug() << "Cannot send message - active wavelength not found";
            return false;
        }

        WavelengthInfo info = registry->getWavelengthInfo(freq);
        QWebSocket* socket = info.socket;

        if (!socket) {
            qDebug() << "Cannot send message - no socket for wavelength" << freq;
            return false;
        }

        if (!socket->isValid()) {
            qDebug() << "Cannot send message - socket for wavelength" << freq << "is invalid";
            return false;
        }

        // Generate a sender ID (use host ID if we're host, or client ID otherwise)
        QString senderId = info.isHost ? info.hostId :
                          AuthenticationManager::getInstance()->generateClientId();

        QString messageId = MessageHandler::getInstance()->generateMessageId();

        // Zapamiętujemy treść wiadomości do późniejszego użycia
        m_sentMessages[messageId] = message;

        // Ograniczamy rozmiar cache'u wiadomości
        if (m_sentMessages.size() > 100) {
            auto it = m_sentMessages.begin();
            m_sentMessages.erase(it);
        }

        // Tworzmy obiekt wiadomości
        QJsonObject messageObj;
        messageObj["type"] = "send_message"; // Używamy typu zgodnego z serwerem
        messageObj["frequency"] = freq;
        messageObj["content"] = message;
        messageObj["senderId"] = senderId;
        messageObj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        messageObj["messageId"] = messageId;

        QJsonDocument doc(messageObj);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending message to server:" << jsonMessage;
        socket->sendTextMessage(jsonMessage);

        qDebug() << "WavelengthMessageService: Message sent:" << messageId;

        return true;
    }

    QMap<QString, QString>* getSentMessageCache() {
        return &m_sentMessages;
    }

    void clearSentMessageCache() {
        m_sentMessages.clear();
    }

    QString getClientId() const {
        return m_clientId;
    }

    void setClientId(const QString& clientId) {
        m_clientId = clientId;
    }

signals:
    void messageSent(double frequency, const QString& formattedMessage);

private:
    WavelengthMessageService(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthMessageService() {}

    WavelengthMessageService(const WavelengthMessageService&) = delete;
    WavelengthMessageService& operator=(const WavelengthMessageService&) = delete;

    QMap<QString, QString> m_sentMessages;
    QString m_clientId;  // Przechowuje ID klienta
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H