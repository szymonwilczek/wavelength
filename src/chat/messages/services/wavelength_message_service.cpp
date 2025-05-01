#include "wavelength_message_service.h"

#include "../../../auth/authentication_manager.h"
#include "../../files/attachments/attachment_queue_manager.h"

bool WavelengthMessageService::SendPttRequest(const QString &frequency) {
    QWebSocket* socket = GetSocketForFrequency(frequency);
    if (!socket) return false;

    QJsonObject request_object;
    request_object["type"] = "request_ptt";
    request_object["frequency"] = frequency;

    const QJsonDocument document(request_object);
    const QString json_message = document.toJson(QJsonDocument::Compact);
    qDebug() << "Sending PTT Request:" << json_message;
    socket->sendTextMessage(json_message);
    return true;
}

bool WavelengthMessageService::SendPttRelease(const QString &frequency) {
    QWebSocket* socket = GetSocketForFrequency(frequency);
    if (!socket) return false;

    QJsonObject release_object;
    release_object["type"] = "release_ptt";
    release_object["frequency"] = frequency;

    const QJsonDocument document(release_object);
    const QString json_message = document.toJson(QJsonDocument::Compact);
    qDebug() << "Sending PTT Release:" << json_message;
    socket->sendTextMessage(json_message);
    return true;
}

bool WavelengthMessageService::SendAudioData(const QString &frequency, const QByteArray &audio_data) {
    QWebSocket* socket = GetSocketForFrequency(frequency);
    if (!socket) return false;

    // Wysyłamy surowe dane binarne
    const qint64 bytes_sent = socket->sendBinaryMessage(audio_data);
    // qDebug() << "Sent" << bytesSent << "bytes of audio data for freq" << frequency;
    return bytes_sent == audio_data.size();
}

bool WavelengthMessageService::SendTextMessage(const QString &message) {
    const WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
    const QString frequency = registry->GetActiveWavelength();

    if (frequency == -1) {
        qDebug() << "Cannot send message - no active wavelength";
        return false;
    }

    if (!registry->HasWavelength(frequency)) {
        qDebug() << "Cannot send message - active wavelength not found";
        return false;
    }

    const WavelengthInfo info = registry->GetWavelengthInfo(frequency);
    QWebSocket* socket = info.socket;

    if (!socket) {
        qDebug() << "Cannot send message - no socket for wavelength" << frequency;
        return false;
    }

    if (!socket->isValid()) {
        qDebug() << "Cannot send message - socket for wavelength" << frequency << "is invalid";
        return false;
    }

    // Generate a sender ID (use host ID if we're host, or client ID otherwise)
    const QString sender_id = info.is_host ? info.host_id :
                           AuthenticationManager::GetInstance()->GenerateClientId();

    const QString message_id = MessageHandler::GetInstance()->GenerateMessageId();

    // Zapamiętujemy treść wiadomości do późniejszego użycia
    sent_messages_[message_id] = message;

    // Ograniczamy rozmiar cache'u wiadomości
    if (sent_messages_.size() > 100) {
        const auto it = sent_messages_.begin();
        sent_messages_.erase(it);
    }

    // Tworzmy obiekt wiadomości
    QJsonObject message_object;
    message_object["type"] = "send_message"; // Używamy typu zgodnego z serwerem
    message_object["frequency"] = frequency;
    message_object["content"] = message;
    message_object["senderId"] = sender_id;
    message_object["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    message_object["messageId"] = message_id;

    const QJsonDocument document(message_object);
    const QString json_message = document.toJson(QJsonDocument::Compact);
    // qDebug() << "Sending message to server:" << jsonMessage;
    socket->sendTextMessage(json_message);

    qDebug() << "WavelengthMessageService: Message sent:" << message_id;

    return true;
}

bool WavelengthMessageService::SendFile(const QString &file_path, const QString &progress_message_id) {
    if (file_path.isEmpty()) {
        emit progressMessageUpdated(progress_message_id, "<span style=\"color:#ff5555;\">Error: Empty file path</span>");
        return false;
    }

    // Początkowa informacja o przygotowywaniu pliku
    emit progressMessageUpdated(progress_message_id,
                                "<span style=\"color:#888888;\">Preparing file...</span>");

    // Tworzenie kopii ścieżki pliku i ID wiadomości, aby uniknąć problemów z wątkami
    QString file_path_copy = file_path;
    QString progress_msg_id_copy = progress_message_id;

    // Przeniesienie całego przetwarzania do osobnego wątku przy użyciu AttachmentQueueManager
    AttachmentQueueManager::GetInstance()->AddTask([this, file_path_copy, progress_msg_id_copy]() {
        try {
            const QFileInfo file_info(file_path_copy);
            if (!file_info.exists() || !file_info.isReadable()) {
                emit progressMessageUpdated(progress_msg_id_copy,
                                            "<span style=\"color:#ff5555;\">Error: File not accessible</span>");
                return;
            }

            QFile file(file_path_copy);
            if (!file.open(QIODevice::ReadOnly)) {
                emit progressMessageUpdated(progress_msg_id_copy,
                                            "<span style=\"color:#ff5555;\">Error: Cannot open file</span>");
                return;
            }

            // Ustalamy typ pliku
            const QString file_extension = file_info.suffix().toLower();
            QString file_type;
            QString mime_type;

            if (QStringList({"jpg", "jpeg", "png", "gif"}).contains(file_extension)) {
                file_type = "image";
                mime_type = "image/" + file_extension;
                if (file_extension == "jpg") mime_type = "image/jpeg";
            } else if (QStringList({"mp3", "wav", "ogg"}).contains(file_extension)) {
                file_type = "audio";
                mime_type = "audio/" + file_extension;
            } else if (QStringList({"mp4", "webm", "avi", "mov"}).contains(file_extension)) {
                file_type = "video";
                mime_type = "video/" + file_extension;
            } else {
                file_type = "file";
                mime_type = "application/octet-stream";
            }

            // Informujemy o rozpoczęciu przetwarzania
            emit progressMessageUpdated(progress_msg_id_copy,
                                        QString("<span style=\"color:#888888;\">Processing %1: %2...</span>")
                                        .arg(file_type)
                                        .arg(file_info.fileName()));

            // Czytamy plik w kawałkach
            QByteArray file_data;
            const qint64 file_size = file_info.size();
            constexpr qint64 chunk_size = 1024 * 1024; // 1 MB chunks
            qint64 total_read = 0;
            int last_progress_reported = 0;

            while (!file.atEnd()) {
                QByteArray chunk = file.read(chunk_size);
                file_data.append(chunk);
                total_read += chunk.size();

                // Aktualizujemy postęp co 5%
                if (file_size > 0) {
                    const int progress = (total_read * 100) / file_size;
                    if (progress - last_progress_reported >= 5) {
                        last_progress_reported = progress;
                        emit progressMessageUpdated(progress_msg_id_copy,
                                                    QString("<span style=\"color:#888888;\">Processing %1: %2... %3%</span>")
                                                    .arg(file_type)
                                                    .arg(file_info.fileName())
                                                    .arg(progress));

                        // Krótkie uśpienie, aby dać szansę innym wątkom
                        QThread::msleep(1);
                    }
                }
            }

            file.close();

            // Informujemy o rozpoczęciu kodowania
            emit progressMessageUpdated(progress_msg_id_copy,
                                        QString("<span style=\"color:#888888;\">Encoding %1: %2...</span>")
                                        .arg(file_type)
                                        .arg(file_info.fileName()));

            // Kodowanie base64
            const QByteArray base64_data = file_data.toBase64();

            // Pobieramy dane potrzebne do wysłania
            const WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
            const QString frequency = registry->GetActiveWavelength();
            const QString sender_id = AuthenticationManager::GetInstance()->GenerateClientId();
            const QString message_id = MessageHandler::GetInstance()->GenerateMessageId();

            // Tworzymy obiekt wiadomości
            QJsonObject message_object;
            message_object["type"] = "send_file";
            message_object["frequency"] = frequency;
            message_object["senderId"] = sender_id;
            message_object["messageId"] = message_id;
            message_object["timestamp"] = QDateTime::currentMSecsSinceEpoch();
            message_object["hasAttachment"] = true;
            message_object["attachmentType"] = file_type;
            message_object["attachmentMimeType"] = mime_type;
            message_object["attachmentName"] = file_info.fileName();
            message_object["attachmentData"] = QString(base64_data);

            // Informujemy o rozpoczęciu wysyłania
            emit progressMessageUpdated(progress_msg_id_copy,
                                        QString("<span style=\"color:#888888;\">Sending %1: %2...</span>")
                                        .arg(file_type)
                                        .arg(file_info.fileName()));

            const QJsonDocument document(message_object);
            const QString json_message = document.toJson(QJsonDocument::Compact);

            // Wysyłamy plik używając sygnału
            emit sendJsonViaSocket(json_message, frequency, progress_msg_id_copy);
        }
        catch (const std::exception& e) {
            emit progressMessageUpdated(progress_msg_id_copy,
                                        QString("<span style=\"color:#ff5555;\">Error: %1</span>").arg(e.what()));
        }
    });

    return true;
}

bool WavelengthMessageService::SendFileToServer(const QString &json_message, const QString &frequency,
    const QString &progress_message_id) {
    const WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
    QWebSocket* socket = nullptr;

    if (registry->HasWavelength(frequency)) {
        const WavelengthInfo info = registry->GetWavelengthInfo(frequency);
        socket = info.socket;
    }

    if (!socket || !socket->isValid()) {
        UpdateProgressMessage(progress_message_id, "<span style=\"color:#ff5555;\">Error: Not connected to server</span>");
        return false;
    }

    // Faktycznie wysyłamy dane
    socket->sendTextMessage(json_message);

    // Potwierdzenie wysłania
    UpdateProgressMessage(progress_message_id, "<span style=\"color:#66cc66;\">File sent successfully!</span>");

    // Po 5 sekundach usuwamy komunikat postępu
    QTimer::singleShot(5000, this, [this, progress_message_id]() {
        emit removeProgressMessage(progress_message_id);
    });

    return true;
}

void WavelengthMessageService::HandleSendJsonViaSocket(const QString &json_message, const QString &frequency,
    const QString &progress_message_id) {
    const WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
    QWebSocket* socket = nullptr;

    if (registry->HasWavelength(frequency)) {
        const WavelengthInfo info = registry->GetWavelengthInfo(frequency);
        socket = info.socket;
    }

    if (!socket || !socket->isValid()) {
        emit progressMessageUpdated(progress_message_id,
                                    "<span style=\"color:#ff5555;\">Error: Not connected to server</span>");
        return;
    }

    // Wysyłamy dane
    socket->sendTextMessage(json_message);

    // Potwierdzamy wysłanie - TO JEST OSTATNIA AKTUALIZACJA WIADOMOŚCI
    emit progressMessageUpdated(progress_message_id,
                                "<span style=\"color:#66cc66;\">File sent successfully!</span>");

}

WavelengthMessageService::WavelengthMessageService(QObject *parent): QObject(parent) {
    // Łączymy sygnał wysyłania przez socket z odpowiednim slotem
    connect(this, &WavelengthMessageService::sendJsonViaSocket,
            this, &WavelengthMessageService::HandleSendJsonViaSocket,
            Qt::QueuedConnection);
}

QWebSocket * WavelengthMessageService::GetSocketForFrequency(const QString &frequency) {
    const WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
    if (!registry->HasWavelength(frequency)) {
        qWarning() << "No wavelength info found for frequency" << frequency << "in getSocketForFrequency";
        return nullptr;
    }
    WavelengthInfo info = registry->GetWavelengthInfo(frequency);
    if (!info.socket || !info.socket->isValid()) {
        qWarning() << "Invalid socket for frequency" << frequency << "in getSocketForFrequency";
        return nullptr;
    }
    return info.socket;
}
