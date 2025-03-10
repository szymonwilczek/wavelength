#ifndef WAVELENGTH_MESSAGE_SERVICE_H
#define WAVELENGTH_MESSAGE_SERVICE_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QString>
#include <QDebug>

#include "../registry/wavelength_registry.h"
#include "../messages/message_handler.h"
#include "../auth/authentication_manager.h"
#include "../messages/message_formatter.h" // Użyj formattera zamiast procesora

class WavelengthMessageService : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageService* getInstance() {
        static WavelengthMessageService instance;
        return &instance;
    }

    bool sendMessage(const QString& message) {
        // Get the active wavelength
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        int frequency = registry->getActiveWavelength();

        if (frequency == -1) {
            qDebug() << "No active wavelength to send message to";
            return false;
        }

        if (!registry->hasWavelength(frequency)) {
            qDebug() << "Wavelength" << frequency << "not found in registry";
            return false;
        }

        WavelengthInfo info = registry->getWavelengthInfo(frequency);

        if (!info.socket || !info.socket->isValid()) {
            qDebug() << "Invalid socket for wavelength" << frequency;
            return false;
        }

        // Generate a message ID
        QString messageId = MessageHandler::getInstance()->generateMessageId();

        // Store the message in the sent message cache
        m_sentMessages[messageId] = message;

        // Clean up cache if it gets too big
        if (m_sentMessages.size() > 100) {
            auto it = m_sentMessages.begin();
            m_sentMessages.erase(it);
        }

        // Create client ID for the sender
        QString clientId;
        if (info.isHost) {
            clientId = info.hostId;
        } else {
            // Tworzenie ID klienta, jeśli nie jest hostem - używamy ID z struktury WavelengthInfo
            // lub generujemy nowe, jeśli potrzeba
            if (!m_clientId.isEmpty()) {
                clientId = m_clientId;
            } else {
                clientId = AuthenticationManager::getInstance()->generateClientId();
                m_clientId = clientId; // Zapamiętanie do późniejszego użycia
            }
        }

        // Send the message
        bool success = MessageHandler::getInstance()->sendMessage(
            info.socket, frequency, message, clientId);

        if (success) {
            // Użyj zewnętrznego formattera
            QString formattedMsg = MessageFormatter::formatSentMessage(frequency, message, info.isHost);
            emit messageSent(frequency, formattedMsg);
        }

        return success;
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
    void messageSent(int frequency, const QString& formattedMessage);

private:
    WavelengthMessageService(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthMessageService() {}

    WavelengthMessageService(const WavelengthMessageService&) = delete;
    WavelengthMessageService& operator=(const WavelengthMessageService&) = delete;

    QMap<QString, QString> m_sentMessages;
    QString m_clientId;  // Przechowuje ID klienta
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H