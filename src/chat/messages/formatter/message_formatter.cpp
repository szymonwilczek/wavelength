#include "message_formatter.h"

#include <QJsonObject>

QString MessageFormatter::formatMessage(const QJsonObject &msgObj, const QString &frequency) {
    // Pobierz treść wiadomości
    QString content;
    if (msgObj.contains("content")) {
        content = msgObj["content"].toString();
    }

    // Pobierz nadawcę
    QString senderName;
    if (msgObj.contains("sender")) {
        senderName = msgObj["sender"].toString();
    } else if (msgObj.contains("senderId")) {
        const QString senderId = msgObj["senderId"].toString();
        const WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);

        if (senderId == info.hostId) {
            senderName = "Host";
        } else {
            senderName = "User " + senderId.left(5);
        }
    }

    // Pobierz timestamp
    QString timestamp;
    if (msgObj.contains("timestamp")) {
        QDateTime msgTime;
        const QVariant timeVar = msgObj["timestamp"].toVariant();
        if (timeVar.type() == QVariant::String) {
            msgTime = QDateTime::fromString(timeVar.toString(), Qt::ISODate);
        } else {
            msgTime = QDateTime::fromMSecsSinceEpoch(timeVar.toLongLong());
        }
        timestamp = msgTime.toString("[HH:mm:ss]");
    } else {
        timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    }

    // Formatowanie wiadomości
    QString formattedMsg;
    const bool isSelf = msgObj.contains("isSelf") && msgObj["isSelf"].toBool();
    const bool hasAttachment = msgObj.contains("hasAttachment") && msgObj["hasAttachment"].toBool();

    QString messageStart;
    if (isSelf) {
        messageStart = QString("%1 <span style=\"color:#60ff8a;\">[You]:</span> ").arg(timestamp);
    } else {
        messageStart = QString("%1 <span style=\"color:#85c4ff;\">[%2]:</span> ").arg(timestamp, senderName);
    }

    if (hasAttachment) {
        const QString attachmentType = msgObj["attachmentType"].toString();
        QString attachmentName = msgObj["attachmentName"].toString();
        QString attachmentMimeType = msgObj["attachmentMimeType"].toString();

        // KLUCZOWA ZMIANA: Przetwarzamy dane załącznika oddzielnie
        QString attachmentId;

        // Sprawdź czy wiadomość zawiera już tylko referencję (placeholder)
        if (!msgObj["attachmentData"].toString().isEmpty() && msgObj["attachmentData"].toString().length() < 100) {
            // Używamy już przechowanego ID
            attachmentId = msgObj["attachmentData"].toString();
        } else if (msgObj.contains("attachmentData") && !msgObj["attachmentData"].toString().isEmpty()) {
            // Zapisz dane base64 w magazynie i uzyskaj ID
            attachmentId = AttachmentDataStore::GetInstance()->StoreAttachmentData(
                msgObj["attachmentData"].toString());
        } else {
            // Brak danych załącznika
            attachmentId = "";
        }

        // Generuj formatowanie bez umieszczania danych base64 w HTML
        if (attachmentType == "image") {
            if (attachmentMimeType == "image/gif") {
                formattedMsg = messageStart + "<br>" +
                               QString("<div class='gif-placeholder' "
                                   "data-gif-id='%1' "
                                   "data-mime-type='%2' "
                                   "data-attachment-id='%3' "
                                   "data-filename='%4'>"
                                   "</div>")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                    attachmentMimeType,
                                    attachmentId,
                                    attachmentName);
            } else {
                formattedMsg = messageStart + "<br>" +
                               QString("<div class='image-placeholder' "
                                   "data-image-id='%1' "
                                   "data-mime-type='%2' "
                                   "data-attachment-id='%3' "
                                   "data-filename='%4'>"
                                   "</div>")
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                    attachmentMimeType,
                                    attachmentId,
                                    attachmentName);
            }
        }
        else if (attachmentType == "audio") {
            formattedMsg = messageStart + "<br>" +
                           QString("<div class='audio-placeholder' "
                               "data-audio-id='%1' "
                               "data-mime-type='%2' "
                               "data-attachment-id='%3' "
                               "data-filename='%4'>"
                               "</div>")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                attachmentMimeType,
                                attachmentId,
                                attachmentName);
        }
        else if (attachmentType == "video") {
            formattedMsg = messageStart + "<br>" +
                           QString("<div class='video-placeholder' "
                               "data-video-id='%1' "
                               "data-mime-type='%2' "
                               "data-attachment-id='%3' "
                               "data-filename='%4'>"
                               "</div>")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces),
                                attachmentMimeType,
                                attachmentId,
                                attachmentName);
        }
        else {
            formattedMsg = messageStart + " " +
                           QString("<a href='file:%1' style='color:#85c4ff;'>[%2]</a>")
                           .arg(attachmentName, attachmentName);
        }
    } else {
        // Zwykła wiadomość tekstowa
        if (content.isEmpty()) {
            content = "[Empty message]";
        }
        content.replace("\n", "<br>");
        formattedMsg = messageStart + "<span style=\"color:#ffffff;\">" + content + "</span>";
    }

    return formattedMsg;
}

QString MessageFormatter::formatFileSize(const qint64 sizeInBytes) {
    if (sizeInBytes < 1024)
        return QString("%1 bytes").arg(sizeInBytes);
    else if (sizeInBytes < 1024 * 1024)
        return QString("%1 KB").arg(sizeInBytes / 1024.0, 0, 'f', 1);
    else if (sizeInBytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(sizeInBytes / (1024.0 * 1024.0), 0, 'f', 2);
    else
        return QString("%1 GB").arg(sizeInBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

QString MessageFormatter::formatSystemMessage(const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    return QString("%1 <span style=\"color:#ffcc00;\">%2</span>").arg(timestamp, message);
}
