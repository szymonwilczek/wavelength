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


    bool sendSystemCommand(QWebSocket* socket, const QString& command, const QJsonObject& params = QJsonObject());

    bool isMessageProcessed(const QString& messageId) {
        return !messageId.isEmpty() && m_processedMessageIds.contains(messageId);
    }

    void markMessageAsProcessed(const QString& messageId);

    QJsonObject parseMessage(const QString& message, bool* ok = nullptr);

    QString getMessageType(const QJsonObject& messageObj) {
        return messageObj["type"].toString();
    }

    QString getMessageContent(const QJsonObject& messageObj) {
        return messageObj["content"].toString();
    }

    QString getMessageFrequency(const QJsonObject& messageObj) {
        return messageObj["frequency"].toString();
    }

    QString getMessageSenderId(const QJsonObject& messageObj) {
        return messageObj["senderId"].toString();
    }

    QString getMessageId(const QJsonObject& messageObj) {
        return messageObj["messageId"].toString();
    }

    QJsonObject createAuthRequest(const QString& frequency, const QString& password, const QString& clientId);


    QJsonObject createRegisterRequest(const QString &frequency, bool isPasswordProtected,
                                      const QString& password, const QString& hostId);

    QJsonObject createLeaveRequest(QString frequency, bool isHost);

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