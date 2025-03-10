#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QSet>
#include <QWebSocket>
#include <QDebug>

class MessageHandler : public QObject {
    Q_OBJECT

public:
    static MessageHandler* getInstance() {
        static MessageHandler instance;
        return &instance;
    }

    QString generateMessageId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }


    bool sendSystemCommand(QWebSocket* socket, const QString& command, const QJsonObject& params = QJsonObject()) {
        if (!socket || !socket->isValid()) {
            qDebug() << "Cannot send command - socket is invalid";
            return false;
        }

        QJsonObject commandObj;
        commandObj["type"] = command;
        
        // Add all parameters from the params object
        for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
            commandObj[it.key()] = it.value();
        }

        QJsonDocument doc(commandObj);
        socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
        
        return true;
    }

    bool isMessageProcessed(const QString& messageId) {
        return !messageId.isEmpty() && m_processedMessageIds.contains(messageId);
    }

    void markMessageAsProcessed(const QString& messageId) {
        if (!messageId.isEmpty()) {
            m_processedMessageIds.insert(messageId);
            
            // Keep the processed message cache from growing too large
            if (m_processedMessageIds.size() > MAX_CACHED_MESSAGE_IDS) {
                // Remove approximately 20% of the oldest messages
                int toRemove = MAX_CACHED_MESSAGE_IDS / 5;
                auto it = m_processedMessageIds.begin();
                for (int i = 0; i < toRemove && it != m_processedMessageIds.end(); ++i) {
                    it = m_processedMessageIds.erase(it);
                }
            }
        }
    }

    QJsonObject parseMessage(const QString& message, bool* ok = nullptr) {
        QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
        if (doc.isNull() || !doc.isObject()) {
            if (ok) *ok = false;
            qDebug() << "Failed to parse message JSON";
            return QJsonObject();
        }
        
        if (ok) *ok = true;
        return doc.object();
    }

    QString getMessageType(const QJsonObject& messageObj) {
        return messageObj["type"].toString();
    }

    QString getMessageContent(const QJsonObject& messageObj) {
        return messageObj["content"].toString();
    }

    int getMessageFrequency(const QJsonObject& messageObj) {
        return messageObj["frequency"].toInt();
    }

    QString getMessageSenderId(const QJsonObject& messageObj) {
        return messageObj["senderId"].toString();
    }

    QString getMessageId(const QJsonObject& messageObj) {
        return messageObj["messageId"].toString();
    }

    QJsonObject createAuthRequest(int frequency, const QString& password, const QString& clientId) {
        QJsonObject authObj;
        authObj["type"] = "auth";
        authObj["frequency"] = frequency;
        authObj["password"] = password;
        authObj["clientId"] = clientId;
        return authObj;
    }

    QJsonObject createRegisterRequest(int frequency, const QString& name, bool isPasswordProtected, 
                                      const QString& password, const QString& hostId) {
        QJsonObject regObj;
        regObj["type"] = "register_wavelength";
        regObj["frequency"] = frequency;
        regObj["name"] = name;
        regObj["isPasswordProtected"] = isPasswordProtected;
        regObj["password"] = password;
        regObj["hostId"] = hostId;
        return regObj;
    }

    QJsonObject createLeaveRequest(int frequency, bool isHost) {
        QJsonObject leaveObj;
        leaveObj["type"] = isHost ? "close_wavelength" : "leave_wavelength";
        leaveObj["frequency"] = frequency;
        return leaveObj;
    }

    void clearProcessedMessages() {
        m_processedMessageIds.clear();
    }

private:
    MessageHandler(QObject* parent = nullptr) : QObject(parent) {}
    ~MessageHandler() {}

    MessageHandler(const MessageHandler&) = delete;
    MessageHandler& operator=(const MessageHandler&) = delete;

    QSet<QString> m_processedMessageIds;
    static const int MAX_CACHED_MESSAGE_IDS = 200;
};

#endif // MESSAGE_HANDLER_H