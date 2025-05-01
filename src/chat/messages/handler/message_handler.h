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
    static MessageHandler* GetInstance() {
        static MessageHandler instance;
        return &instance;
    }

    static QString GenerateMessageId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }


    static bool SendSystemCommand(QWebSocket* socket, const QString& command, const QJsonObject& params = QJsonObject());

    bool IsMessageProcessed(const QString& message_id) const {
        return !message_id.isEmpty() && processed_message_ids_.contains(message_id);
    }

    void MarkMessageAsProcessed(const QString& message_id);

    static QJsonObject ParseMessage(const QString& message, bool* ok = nullptr);

    static QString GetMessageType(const QJsonObject& message_object) {
        return message_object["type"].toString();
    }

    static QString GetMessageContent(const QJsonObject& message_object) {
        return message_object["content"].toString();
    }

    static QString GetMessageFrequency(const QJsonObject& message_object) {
        return message_object["frequency"].toString();
    }

    static QString GetMessageSenderId(const QJsonObject& message_object) {
        return message_object["senderId"].toString();
    }

    static QString GetMessageId(const QJsonObject& message_object) {
        return message_object["messageId"].toString();
    }

    static QJsonObject CreateAuthRequest(const QString& frequency, const QString& password, const QString& client_id);


    static QJsonObject CreateRegisterRequest(const QString &frequency, bool is_password_protected,
                                             const QString& password, const QString& host_id);

    static QJsonObject CreateLeaveRequest(const QString &frequency, bool is_host);

    void ClearProcessedMessages() {
        processed_message_ids_.clear();
    }

private:
    explicit MessageHandler(QObject* parent = nullptr) : QObject(parent) {}
    ~MessageHandler() override {}

    MessageHandler(const MessageHandler&) = delete;
    MessageHandler& operator=(const MessageHandler&) = delete;

    QSet<QString> processed_message_ids_;
    static constexpr int kMaxCachedMessageIds = 200;
};

#endif // MESSAGE_HANDLER_H