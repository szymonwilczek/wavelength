#ifndef WAVELENGTH_MESSAGE_SERVICE_H
#define WAVELENGTH_MESSAGE_SERVICE_H

#include <QtConcurrent>

#include "../../../storage/wavelength_registry.h"
#include "../handler/message_handler.h"

class WavelengthMessageService : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageService* getInstance() {
        static WavelengthMessageService instance;
        return &instance;
    }

    // --- NOWE METODY PTT ---
    bool sendPttRequest(const QString& frequency);

    bool sendPttRelease(const QString& frequency);

    bool sendAudioData(const QString& frequency, const QByteArray& audioData);

    bool sendMessage(const QString& message);

    bool sendFileMessage(const QString& filePath, const QString& progressMsgId = QString());

    // Dodaj nową wersję metody sendFileToServer, która zwraca bool
bool sendFileToServer(const QString& jsonMessage, QString frequency, const QString& progressMsgId);


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

    public slots:
    void updateProgressMessage(const QString& progressMsgId, const QString& message) {
        if (progressMsgId.isEmpty()) return;
        // Ten sygnał jest podłączony do WavelengthChatView::updateProgressMessage,
        // który wywoła addMessage, a ten z kolei StreamMessage::updateContent
        emit progressMessageUpdated(progressMsgId, message);
    }

    // Dodajemy slot do wysyłania wiadomości przez socket
    void handleSendJsonViaSocket(const QString& jsonMessage, QString frequency, const QString& progressMsgId);

signals:
    void messageSent(QString frequency, const QString& formattedMessage);
    void progressMessageUpdated(const QString& messageId, const QString& message);
    void removeProgressMessage(const QString& messageId);
    void sendJsonViaSocket(const QString& jsonMessage, QString frequency, const QString& progressMsgId);
    void pttGranted(QString frequency);
    void pttDenied(QString frequency, QString reason);
    void pttStartReceiving(QString frequency, QString senderId);
    void pttStopReceiving(QString frequency);
    void audioDataReceived(QString frequency, const QByteArray& audioData);
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private:
    WavelengthMessageService(QObject* parent = nullptr);

    ~WavelengthMessageService() {}

    WavelengthMessageService(const WavelengthMessageService&) = delete;
    WavelengthMessageService& operator=(const WavelengthMessageService&) = delete;

    QMap<QString, QString> m_sentMessages;
    QString m_clientId;

    static QWebSocket* getSocketForFrequency(const QString& frequency);
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H