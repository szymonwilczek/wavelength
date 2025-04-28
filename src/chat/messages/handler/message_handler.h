#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <QObject>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QSet>
#include <QWebSocket>

class MessageHandler final : public QObject {
    Q_OBJECT

public:
    static MessageHandler* getInstance() {
        static MessageHandler instance;
        return &instance;
    }

    static QString generateMessageId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }


    static bool sendSystemCommand(QWebSocket* socket, const QString& command, const QJsonObject& params = QJsonObject());

    bool isMessageProcessed(const QString& messageId) const {
        return !messageId.isEmpty() && m_processedMessageIds.contains(messageId);
    }

    void markMessageAsProcessed(const QString& messageId);

    static QJsonObject parseMessage(const QString& message, bool* ok = nullptr);

    static QString getMessageType(const QJsonObject& messageObj) {
        return messageObj["type"].toString();
    }

    static QString getMessageContent(const QJsonObject& messageObj) {
        return messageObj["content"].toString();
    }

    static QString getMessageFrequency(const QJsonObject& messageObj) {
        return messageObj["frequency"].toString();
    }

    static QString getMessageSenderId(const QJsonObject& messageObj) {
        return messageObj["senderId"].toString();
    }

    static QString getMessageId(const QJsonObject& messageObj) {
        return messageObj["messageId"].toString();
    }

    static QJsonObject createAuthRequest(const QString& frequency, const QString& password, const QString& clientId);


    static QJsonObject createRegisterRequest(const QString &frequency, bool isPasswordProtected,
                                             const QString& password, const QString& hostId);

    static QJsonObject createLeaveRequest(const QString &frequency, bool isHost);

    void clearProcessedMessages() {
        m_processedMessageIds.clear();
    }

private:
    explicit MessageHandler(QObject* parent = nullptr) : QObject(parent) {}
    ~MessageHandler() override {}

    MessageHandler(const MessageHandler&) = delete;
    MessageHandler& operator=(const MessageHandler&) = delete;

    QSet<QString> m_processedMessageIds;
    static constexpr int MAX_CACHED_MESSAGE_IDS = 200;
};

#endif // MESSAGE_HANDLER_H