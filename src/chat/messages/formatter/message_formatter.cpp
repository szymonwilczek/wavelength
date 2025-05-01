#include "message_formatter.h"

#include <QJsonObject>

QString MessageFormatter::FormatMessage(const QJsonObject &message_object, const QString &frequency) {
    QString content;
    if (message_object.contains("content")) {
        content = message_object["content"].toString();
    }

    QString sender_name;
    if (message_object.contains("sender")) {
        sender_name = message_object["sender"].toString();
    } else if (message_object.contains("senderId")) {
        const QString sender_id = message_object["senderId"].toString();
        const WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);

        if (sender_id == info.hostId) {
            sender_name = "Host";
        } else {
            sender_name = "User " + sender_id.left(5);
        }
    }

    // Pobierz timestamp
    QString timestamp;
    if (message_object.contains("timestamp")) {
        QDateTime message_type;
        const QVariant timeVar = message_object["timestamp"].toVariant();
        if (timeVar.type() == QVariant::String) {
            message_type = QDateTime::fromString(timeVar.toString(), Qt::ISODate);
        } else {
            message_type = QDateTime::fromMSecsSinceEpoch(timeVar.toLongLong());
        }
        timestamp = message_type.toString("[HH:mm:ss]");
    } else {
        timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    }

    // Formatowanie wiadomości
    QString formatted_message;
    const bool is_self = message_object.contains("isSelf") && message_object["isSelf"].toBool();
    const bool has_attachment = message_object.contains("hasAttachment") && message_object["hasAttachment"].toBool();

    QString message_start;
    if (is_self) {
        message_start = QString("%1 <span style=\"color:#60ff8a;\">[You]:</span> ").arg(timestamp);
    } else {
        message_start = QString("%1 <span style=\"color:#85c4ff;\">[%2]:</span> ").arg(timestamp, sender_name);
    }

    if (has_attachment) {
        const QString attachment_type = message_object["attachmentType"].toString();
        QString attachment_name = message_object["attachmentName"].toString();
        QString attachment_mime_type = message_object["attachmentMimeType"].toString();

        // KLUCZOWA ZMIANA: Przetwarzamy dane załącznika oddzielnie
        QString attachment_id;

        // Sprawdź czy wiadomość zawiera już tylko referencję (placeholder)
        if (!message_object["attachmentData"].toString().isEmpty() && message_object["attachmentData"].toString().length() < 100) {
            // Używamy już przechowanego ID
            attachment_id = message_object["attachmentData"].toString();
        } else if (message_object.contains("attachmentData") && !message_object["attachmentData"].toString().isEmpty()) {
            // Zapisz dane base64 w magazynie i uzyskaj ID
            attachment_id = AttachmentDataStore::GetInstance()->StoreAttachmentData(
                message_object["attachmentData"].toString());
        } else {
            // Brak danych załącznika
            attachment_id = "";
        }

        // Generuj formatowanie bez umieszczania danych base64 w HTML
        if (attachment_type == "image") {
            if (attachment_mime_type == "image/gif") {
                formatted_message = message_start + "<br>" +
                               QString("<div class='gif-placeholder' "
                                   "data-gif-id='%1' "
                                   "data-mime-type='%2' "
                                   "data-attachment-id='%3' "
                                   "data-filename='%4'>"
                                   "</div>")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                    attachment_mime_type,
                                    attachment_id,
                                    attachment_name);
            } else {
                formatted_message = message_start + "<br>" +
                               QString("<div class='image-placeholder' "
                                   "data-image-id='%1' "
                                   "data-mime-type='%2' "
                                   "data-attachment-id='%3' "
                                   "data-filename='%4'>"
                                   "</div>")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                    attachment_mime_type,
                                    attachment_id,
                                    attachment_name);
            }
        }
        else if (attachment_type == "audio") {
            formatted_message = message_start + "<br>" +
                           QString("<div class='audio-placeholder' "
                               "data-audio-id='%1' "
                               "data-mime-type='%2' "
                               "data-attachment-id='%3' "
                               "data-filename='%4'>"
                               "</div>")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                attachment_mime_type,
                                attachment_id,
                                attachment_name);
        }
        else if (attachment_type == "video") {
            formatted_message = message_start + "<br>" +
                           QString("<div class='video-placeholder' "
                               "data-video-id='%1' "
                               "data-mime-type='%2' "
                               "data-attachment-id='%3' "
                               "data-filename='%4'>"
                               "</div>")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                attachment_mime_type,
                                attachment_id,
                                attachment_name);
        }
        else {
            formatted_message = message_start + " " +
                           QString("<a href='file:%1' style='color:#85c4ff;'>[%2]</a>")
                           .arg(attachment_name, attachment_name);
        }
    } else {
        // Zwykła wiadomość tekstowa
        if (content.isEmpty()) {
            content = "[Empty message]";
        }
        content.replace("\n", "<br>");
        formatted_message = message_start + "<span style=\"color:#ffffff;\">" + content + "</span>";
    }

    return formatted_message;
}

QString MessageFormatter::FormatFileSize(const qint64 size_in_bytes) {
    if (size_in_bytes < 1024)
        return QString("%1 bytes").arg(size_in_bytes);
    if (size_in_bytes < 1024 * 1024)
        return QString("%1 KB").arg(size_in_bytes / 1024.0, 0, 'f', 1);
    if (size_in_bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(size_in_bytes / (1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 GB").arg(size_in_bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

QString MessageFormatter::FormatSystemMessage(const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    return QString("%1 <span style=\"color:#ffcc00;\">%2</span>").arg(timestamp, message);
}
