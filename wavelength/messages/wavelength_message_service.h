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

    bool sendFileMessage(const QString& filePath, const QString& progressMsgId = QString()) {
    // Tworzymy kopię ścieżki, którą przekażemy do wątku
    QString filePathCopy = filePath;

    // Uruchamiamy asynchroniczne przetwarzanie pliku
    QFuture<void> future = QtConcurrent::run([this, filePathCopy, progressMsgId]() {
        WavelengthRegistry* registry = WavelengthRegistry::getInstance();
        double frequency = registry->getActiveWavelength();

        if (filePathCopy.isEmpty()) {
            qDebug() << "Empty file path";
            this->updateProgressMessage(progressMsgId, "<span style=\"color:#ff5555;\">Error: Empty file path</span>");
            return;
        }

        QFileInfo fileInfo(filePathCopy);
        if (!fileInfo.exists() || !fileInfo.isReadable()) {
            qDebug() << "File does not exist or is not readable:" << filePathCopy;
            this->updateProgressMessage(progressMsgId, "<span style=\"color:#ff5555;\">Error: File not accessible</span>");
            return;
        }

        QFile file(filePathCopy);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Cannot open file for reading:" << filePathCopy;
            this->updateProgressMessage(progressMsgId, "<span style=\"color:#ff5555;\">Error: Cannot open file</span>");
            return;
        }

        QString fileExtension = fileInfo.suffix().toLower();
        QString fileType;
        QString mimeType;

        // Ustalamy typ pliku i MIME
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

        // Aktualizacja komunikatu przed wczytywaniem dużego pliku
        this->updateProgressMessage(progressMsgId,
            QString("<span style=\"color:#888888;\">Processing %1: %2...</span>")
            .arg(fileType)
            .arg(fileInfo.fileName()));

        // Czytamy zawartość pliku w kawałkach dla dużych plików
        QByteArray fileData;
        qint64 fileSize = fileInfo.size();
        const qint64 chunkSize = 1024 * 1024; // 1 MB chunks
        qint64 totalRead = 0;

        while (!file.atEnd()) {
            QByteArray chunk = file.read(chunkSize);
            fileData.append(chunk);
            totalRead += chunk.size();

            // Aktualizacja postępu co jakiś czas
            if (progressMsgId != QString() && fileSize > 0) {
                int progress = (totalRead * 100) / fileSize;
                if (progress % 10 == 0) { // Aktualizuj co 10%
                    this->updateProgressMessage(progressMsgId,
                        QString("<span style=\"color:#888888;\">Processing %1: %2... %3%</span>")
                        .arg(fileType)
                        .arg(fileInfo.fileName())
                        .arg(progress));
                }
            }
        }

        file.close();

        // Aktualizacja przed kodowaniem Base64
        this->updateProgressMessage(progressMsgId,
            QString("<span style=\"color:#888888;\">Encoding %1: %2...</span>")
            .arg(fileType)
            .arg(fileInfo.fileName()));

        QByteArray base64Data = fileData.toBase64();

        // Tworzymy obiekt wiadomości
        QJsonObject messageObj;
        messageObj["type"] = "send_file";
        messageObj["frequency"] = frequency;
        messageObj["senderId"] = AuthenticationManager::getInstance()->generateClientId();
        messageObj["messageId"] = MessageHandler::getInstance()->generateMessageId();
        messageObj["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        messageObj["hasAttachment"] = true;
        messageObj["attachmentType"] = fileType;
        messageObj["attachmentMimeType"] = mimeType;
        messageObj["attachmentName"] = fileInfo.fileName();
        messageObj["attachmentData"] = QString(base64Data);

        // Aktualizacja przed wysłaniem
        this->updateProgressMessage(progressMsgId,
            QString("<span style=\"color:#888888;\">Sending %1: %2...</span>")
            .arg(fileType)
            .arg(fileInfo.fileName()));

        QJsonDocument doc(messageObj);
        QString jsonMessage = doc.toJson(QJsonDocument::Compact);

        // To musi być wykonane w głównym wątku
        QMetaObject::invokeMethod(this, "sendFileToServer",
            Qt::QueuedConnection,
            Q_ARG(QString, jsonMessage),
            Q_ARG(double, frequency),
            Q_ARG(QString, progressMsgId));
    });

    return true;
}

    Q_INVOKABLE void sendFileToServer(const QString& jsonMessage, double frequency, const QString& progressMsgId) {
        QWebSocket* socket = WavelengthRegistry::getInstance()->getWavelengthSocket(frequency);
        if (!socket) {
            qDebug() << "No socket for frequency" << frequency;
            updateProgressMessage(progressMsgId, "<span style=\"color:#ff5555;\">Error: Connection lost</span>");
            return;
        }

        socket->sendTextMessage(jsonMessage);

        // Usuwamy komunikat o postępie - nie jest już potrzebny
        if (!progressMsgId.isEmpty()) {
            QTimer::singleShot(500, this, [this, progressMsgId]() {
                emit removeProgressMessage(progressMsgId);
            });
        }
    }

    // Metoda do aktualizacji komunikatu o postępie
    void updateProgressMessage(const QString& progressMsgId, const QString& message) {
        if (!progressMsgId.isEmpty()) {
            emit progressMessageUpdated(progressMsgId, message);
        }
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
    void progressMessageUpdated(const QString& messageId, const QString& message);
    void removeProgressMessage(const QString& messageId);

private:
    WavelengthMessageService(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthMessageService() {}

    WavelengthMessageService(const WavelengthMessageService&) = delete;
    WavelengthMessageService& operator=(const WavelengthMessageService&) = delete;

    QMap<QString, QString> m_sentMessages;
    QString m_clientId;  // Przechowuje ID klienta
};

#endif // WAVELENGTH_MESSAGE_SERVICE_H