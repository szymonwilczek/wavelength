#include "wavelength_message_service.h"

#include "../../../auth/authentication_manager.h"
#include "../../files/attachments/attachment_queue_manager.h"

bool WavelengthMessageService::sendPttRequest(const QString &frequency) {
    QWebSocket* socket = getSocketForFrequency(frequency);
    if (!socket) return false;

    QJsonObject requestObj;
    requestObj["type"] = "request_ptt";
    requestObj["frequency"] = frequency;
    // Można dodać ID sesji klienta, jeśli serwer tego wymaga
    // requestObj["senderId"] = AuthenticationManager::getInstance()->getClientId();

    const QJsonDocument doc(requestObj);
    const QString jsonMessage = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Sending PTT Request:" << jsonMessage;
    socket->sendTextMessage(jsonMessage);
    return true;
}

bool WavelengthMessageService::sendPttRelease(const QString &frequency) {
    QWebSocket* socket = getSocketForFrequency(frequency);
    if (!socket) return false;

    QJsonObject releaseObj;
    releaseObj["type"] = "release_ptt";
    releaseObj["frequency"] = frequency;

    const QJsonDocument doc(releaseObj);
    const QString jsonMessage = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Sending PTT Release:" << jsonMessage;
    socket->sendTextMessage(jsonMessage);
    return true;
}

bool WavelengthMessageService::sendAudioData(const QString &frequency, const QByteArray &audioData) {
    QWebSocket* socket = getSocketForFrequency(frequency);
    if (!socket) return false;

    // Wysyłamy surowe dane binarne
    const qint64 bytesSent = socket->sendBinaryMessage(audioData);
    // qDebug() << "Sent" << bytesSent << "bytes of audio data for freq" << frequency;
    return bytesSent == audioData.size();
}

bool WavelengthMessageService::sendMessage(const QString &message) {
    const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    const QString freq = registry->getActiveWavelength();

    if (freq == -1) {
        qDebug() << "Cannot send message - no active wavelength";
        return false;
    }

    if (!registry->hasWavelength(freq)) {
        qDebug() << "Cannot send message - active wavelength not found";
        return false;
    }

    const WavelengthInfo info = registry->getWavelengthInfo(freq);
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
    const QString senderId = info.isHost ? info.hostId :
                           AuthenticationManager::GetInstance()->GenerateClientId();

    const QString messageId = MessageHandler::getInstance()->generateMessageId();

    // Zapamiętujemy treść wiadomości do późniejszego użycia
    m_sentMessages[messageId] = message;

    // Ograniczamy rozmiar cache'u wiadomości
    if (m_sentMessages.size() > 100) {
        const auto it = m_sentMessages.begin();
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

    const QJsonDocument doc(messageObj);
    const QString jsonMessage = doc.toJson(QJsonDocument::Compact);
    // qDebug() << "Sending message to server:" << jsonMessage;
    socket->sendTextMessage(jsonMessage);

    qDebug() << "WavelengthMessageService: Message sent:" << messageId;

    return true;
}

bool WavelengthMessageService::sendFileMessage(const QString &filePath, const QString &progressMsgId) {
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
            const QFileInfo fileInfo(filePathCopy);
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
            const QString fileExtension = fileInfo.suffix().toLower();
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
            const qint64 fileSize = fileInfo.size();
            constexpr qint64 chunkSize = 1024 * 1024; // 1 MB chunks
            qint64 totalRead = 0;
            int lastProgressReported = 0;

            while (!file.atEnd()) {
                QByteArray chunk = file.read(chunkSize);
                fileData.append(chunk);
                totalRead += chunk.size();

                // Aktualizujemy postęp co 5%
                if (fileSize > 0) {
                    const int progress = (totalRead * 100) / fileSize;
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
            const QByteArray base64Data = fileData.toBase64();

            // Pobieramy dane potrzebne do wysłania
            const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
            const QString frequency = registry->getActiveWavelength();
            const QString senderId = AuthenticationManager::GetInstance()->GenerateClientId();
            const QString messageId = MessageHandler::getInstance()->generateMessageId();

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

            const QJsonDocument doc(messageObj);
            const QString jsonMessage = doc.toJson(QJsonDocument::Compact);

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

bool WavelengthMessageService::sendFileToServer(const QString &jsonMessage, const QString &frequency,
    const QString &progressMsgId) {
    const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    QWebSocket* socket = nullptr;

    if (registry->hasWavelength(frequency)) {
        const WavelengthInfo info = registry->getWavelengthInfo(frequency);
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

void WavelengthMessageService::handleSendJsonViaSocket(const QString &jsonMessage, const QString &frequency,
    const QString &progressMsgId) {
    const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    QWebSocket* socket = nullptr;

    if (registry->hasWavelength(frequency)) {
        const WavelengthInfo info = registry->getWavelengthInfo(frequency);
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

WavelengthMessageService::WavelengthMessageService(QObject *parent): QObject(parent) {
    // Łączymy sygnał wysyłania przez socket z odpowiednim slotem
    connect(this, &WavelengthMessageService::sendJsonViaSocket,
            this, &WavelengthMessageService::handleSendJsonViaSocket,
            Qt::QueuedConnection);
}

QWebSocket * WavelengthMessageService::getSocketForFrequency(const QString &frequency) {
    const WavelengthRegistry* registry = WavelengthRegistry::getInstance();
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
