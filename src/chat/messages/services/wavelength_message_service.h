#ifndef WAVELENGTH_MESSAGE_SERVICE_H
#define WAVELENGTH_MESSAGE_SERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <QString>
#include <QDebug>
#include <QFile>
#include <qfileinfo.h>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

#include "../../../storage/wavelength_registry.h"
#include "../handler/message_handler.h"
#include "../../../auth/authentication_manager.h"
#include "../../../chat/files/attachments/attachment_queue_manager.h"

class WavelengthMessageService : public QObject {
    Q_OBJECT

public:
    static WavelengthMessageService* getInstance() {
        static WavelengthMessageService instance;
        return &instance;
    }

    // --- NOWE METODY PTT ---
    bool sendPttRequest(const QString& frequency) {
        QWebSocket* socket = getSocketForFrequency(frequency);
        if (!socket) return false;

        QJsonObject requestObj;
        requestObj["type"] = "request_ptt";
        requestObj["frequency"] = frequency;
        // Można dodać ID sesji klienta, jeśli serwer tego wymaga
        // requestObj["senderId"] = AuthenticationManager::getInstance()->getClientId();

        QJsonDocument doc(requestObj);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending PTT Request:" << jsonMessage;
        socket->sendTextMessage(jsonMessage);
        return true;
    }

    bool sendPttRelease(const QString& frequency) {
        QWebSocket* socket = getSocketForFrequency(frequency);
        if (!socket) return false;

        QJsonObject releaseObj;
        releaseObj["type"] = "release_ptt";
        releaseObj["frequency"] = frequency;

        QJsonDocument doc(releaseObj);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending PTT Release:" << jsonMessage;
        socket->sendTextMessage(jsonMessage);
        return true;
    }

    bool sendAudioData(const QString& frequency, const QByteArray& audioData) {
        QWebSocket* socket = getSocketForFrequency(frequency);
        if (!socket) return false;

        // Wysyłamy surowe dane binarne
        qint64 bytesSent = socket->sendBinaryMessage(audioData);
        // qDebug() << "Sent" << bytesSent << "bytes of audio data for freq" << frequency;
        return bytesSent == audioData.size();
    }

    bool sendMessage(const QString& message) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        QString freq = registry->getActiveWavelength();

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
        // qDebug() << "Sending message to server:" << jsonMessage;
        socket->sendTextMessage(jsonMessage);

        qDebug() << "WavelengthMessageService: Message sent:" << messageId;

        return true;
    }

    bool sendFileMessage(const QString& filePath, const QString& progressMsgId = QString()) {
    if (filePath.isEmpty()) {
        emit progressMessageUpdated(progressMsgId, "<span style=\"color:#ff5555;\">Error: Empty file path</span>");
        return false;
    }

    // Początkowa informacja o przygotowywaniu pliku
    emit progressMessageUpdated(progressMsgId,
        "<span style=\"color:#888888;\">Preparing file...</span>");

    // Tworzenie kopii ścieżki pliku i ID wiadomości, aby uniknąć problemów z wątkami
    QString filePathCopy = filePath;
    QString progressMsgIdCopy = progressMsgId;

    // Przeniesienie całego przetwarzania do osobnego wątku przy użyciu AttachmentQueueManager
    AttachmentQueueManager::getInstance()->addTask([this, filePathCopy, progressMsgIdCopy]() {
        try {
            QFileInfo fileInfo(filePathCopy);
            if (!fileInfo.exists() || !fileInfo.isReadable()) {
                emit progressMessageUpdated(progressMsgIdCopy,
                    "<span style=\"color:#ff5555;\">Error: File not accessible</span>");
                return;
            }

            QFile file(filePathCopy);
            if (!file.open(QIODevice::ReadOnly)) {
                emit progressMessageUpdated(progressMsgIdCopy,
                    "<span style=\"color:#ff5555;\">Error: Cannot open file</span>");
                return;
            }

            // Ustalamy typ pliku
            QString fileExtension = fileInfo.suffix().toLower();
            QString fileType;
            QString mimeType;

            if (QStringList({"jpg", "jpeg", "png", "gif"}).contains(fileExtension)) {
                fileType = "image";
                mimeType = "image/" + fileExtension;
                if (fileExtension == "jpg") mimeType = "image/jpeg";
            } else if (QStringList({"mp3", "wav", "ogg"}).contains(fileExtension)) {
                fileType = "audio";
                mimeType = "audio/" + fileExtension;
            } else if (QStringList({"mp4", "webm", "avi", "mov"}).contains(fileExtension)) {
                fileType = "video";
                mimeType = "video/" + fileExtension;
            } else {
                fileType = "file";
                mimeType = "application/octet-stream";
            }

            // Informujemy o rozpoczęciu przetwarzania
            emit progressMessageUpdated(progressMsgIdCopy,
                QString("<span style=\"color:#888888;\">Processing %1: %2...</span>")
                .arg(fileType)
                .arg(fileInfo.fileName()));

            // Czytamy plik w kawałkach
            QByteArray fileData;
            qint64 fileSize = fileInfo.size();
            const qint64 chunkSize = 1024 * 1024; // 1 MB chunks
            qint64 totalRead = 0;
            int lastProgressReported = 0;

            while (!file.atEnd()) {
                QByteArray chunk = file.read(chunkSize);
                fileData.append(chunk);
                totalRead += chunk.size();

                // Aktualizujemy postęp co 5%
                if (fileSize > 0) {
                    int progress = (totalRead * 100) / fileSize;
                    if (progress - lastProgressReported >= 5) {
                        lastProgressReported = progress;
                        emit progressMessageUpdated(progressMsgIdCopy,
                            QString("<span style=\"color:#888888;\">Processing %1: %2... %3%</span>")
                            .arg(fileType)
                            .arg(fileInfo.fileName())
                            .arg(progress));

                        // Krótkie uśpienie, aby dać szansę innym wątkom
                        QThread::msleep(1);
                    }
                }
            }

            file.close();

            // Informujemy o rozpoczęciu kodowania
            emit progressMessageUpdated(progressMsgIdCopy,
                QString("<span style=\"color:#888888;\">Encoding %1: %2...</span>")
                .arg(fileType)
                .arg(fileInfo.fileName()));

            // Kodowanie base64
            QByteArray base64Data = fileData.toBase64();

            // Pobieramy dane potrzebne do wysłania
            WavelengthRegistry* registry = WavelengthRegistry::getInstance();
            QString frequency = registry->getActiveWavelength();
            QString senderId = AuthenticationManager::getInstance()->generateClientId();
            QString messageId = MessageHandler::getInstance()->generateMessageId();

            // Tworzymy obiekt wiadomości
            QJsonObject messageObj;
            messageObj["type"] = "send_file";
            messageObj["frequency"] = frequency;
            messageObj["senderId"] = senderId;
            messageObj["messageId"] = messageId;
            messageObj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            messageObj["hasAttachment"] = true;
            messageObj["attachmentType"] = fileType;
            messageObj["attachmentMimeType"] = mimeType;
            messageObj["attachmentName"] = fileInfo.fileName();
            messageObj["attachmentData"] = QString(base64Data);

            // Informujemy o rozpoczęciu wysyłania
            emit progressMessageUpdated(progressMsgIdCopy,
                QString("<span style=\"color:#888888;\">Sending %1: %2...</span>")
                .arg(fileType)
                .arg(fileInfo.fileName()));

            QJsonDocument doc(messageObj);
            QString jsonMessage = doc.toJson(QJsonDocument::Compact);

            // Wysyłamy plik używając sygnału
            emit sendJsonViaSocket(jsonMessage, frequency, progressMsgIdCopy);
        }
        catch (const std::exception& e) {
            emit progressMessageUpdated(progressMsgIdCopy,
                QString("<span style=\"color:#ff5555;\">Error: %1</span>").arg(e.what()));
        }
    });

    return true;
}

// Dodaj nową wersję metody sendFileToServer, która zwraca bool
bool sendFileToServer(const QString& jsonMessage, QString frequency, const QString& progressMsgId) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    QWebSocket* socket = nullptr;

    if (registry->hasWavelength(frequency)) {
        WavelengthInfo info = registry->getWavelengthInfo(frequency);
        socket = info.socket;
    }

    if (!socket || !socket->isValid()) {
        updateProgressMessage(progressMsgId, "<span style=\"color:#ff5555;\">Error: Not connected to server</span>");
        return false;
    }

    // Faktycznie wysyłamy dane
    socket->sendTextMessage(jsonMessage);

    // Potwierdzenie wysłania
    updateProgressMessage(progressMsgId, "<span style=\"color:#66cc66;\">File sent successfully!</span>");

    // Po 5 sekundach usuwamy komunikat postępu
    QTimer::singleShot(5000, this, [this, progressMsgId]() {
        emit removeProgressMessage(progressMsgId);
    });

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

    public slots:
    void updateProgressMessage(const QString& progressMsgId, const QString& message) {
        if (progressMsgId.isEmpty()) return;
        // Ten sygnał jest podłączony do WavelengthChatView::updateProgressMessage,
        // który wywoła addMessage, a ten z kolei StreamMessage::updateContent
        emit progressMessageUpdated(progressMsgId, message);
    }

    // Dodajemy slot do wysyłania wiadomości przez socket
    void handleSendJsonViaSocket(const QString& jsonMessage, QString frequency, const QString& progressMsgId) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        QWebSocket* socket = nullptr;

        if (registry->hasWavelength(frequency)) {
            WavelengthInfo info = registry->getWavelengthInfo(frequency);
            socket = info.socket;
        }

        if (!socket || !socket->isValid()) {
            emit progressMessageUpdated(progressMsgId,
                "<span style=\"color:#ff5555;\">Error: Not connected to server</span>");
            return;
        }

        // Wysyłamy dane
        socket->sendTextMessage(jsonMessage);

        // Potwierdzamy wysłanie - TO JEST OSTATNIA AKTUALIZACJA WIADOMOŚCI
        emit progressMessageUpdated(progressMsgId,
            "<span style=\"color:#66cc66;\">File sent successfully!</span>");

        // USUWAMY TIMER - wiadomość ma pozostać z ostatnim statusem
        // QTimer::singleShot(5000, this, [this, progressMsgId]() {
        //     emit removeProgressMessage(progressMsgId);
        // });
    }

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
    WavelengthMessageService(QObject* parent = nullptr) : QObject(parent) {
        // Łączymy sygnał wysyłania przez socket z odpowiednim slotem
        connect(this, &WavelengthMessageService::sendJsonViaSocket,
                this, &WavelengthMessageService::handleSendJsonViaSocket,
                Qt::QueuedConnection);
    }

    ~WavelengthMessageService() {}

    WavelengthMessageService(const WavelengthMessageService&) = delete;
    WavelengthMessageService& operator=(const WavelengthMessageService&) = delete;

    QMap<QString, QString> m_sentMessages;
    QString m_clientId;

    QWebSocket* getSocketForFrequency(const QString& frequency) {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        if (!registry->hasWavelength(frequency)) {
            qWarning() << "No wavelength info found for frequency" << frequency << "in getSocketForFrequency";
            return nullptr;
        }
        WavelengthInfo info = registry->getWavelengthInfo(frequency);
        if (!info.socket || !info.socket->isValid()) {
            qWarning() << "Invalid socket for frequency" << frequency << "in getSocketForFrequency";
            return nullptr;
        }
        return info.socket;
    }
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H