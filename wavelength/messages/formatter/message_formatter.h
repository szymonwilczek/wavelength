#ifndef MESSAGE_FORMATTER_H
#define MESSAGE_FORMATTER_H

#include <QJsonObject>
#include "../../registry/wavelength_registry.h"

class MessageFormatter {
public:
    static QString formatMessage(const QJsonObject& msgObj, double frequency) {
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
            QString senderId = msgObj["senderId"].toString();
            WavelengthInfo info = WavelengthRegistry::getInstance()->getWavelengthInfo(frequency);

            if (senderId == info.hostId) {
                senderName = "Host";
            } else {
                senderName = "User " + senderId.left(5);
            }
        }

        // Pobierz timestamp
        QString timestamp;
        if (msgObj.contains("timestamp")) {
            // Konwersja do QDateTime
            QDateTime msgTime;
            QVariant timeVar = msgObj["timestamp"].toVariant();
            if (timeVar.type() == QVariant::String) {
                // Format ISO
                QString timeStr = timeVar.toString();
                msgTime = QDateTime::fromString(timeStr, Qt::ISODate);
            } else {
                // Unix timestamp
                msgTime = QDateTime::fromMSecsSinceEpoch(timeVar.toLongLong());
            }

            // Formatuj timestamp
            timestamp = msgTime.toString("[HH:mm:ss]");
        } else {
            // Jeśli brak timestamp, użyj aktualnego czasu
            timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
        }

        // Jednolite formatowanie
        QString formattedMsg;
        bool isSelf = msgObj.contains("isSelf") && msgObj["isSelf"].toBool();

        // Sprawdź czy jest załącznik
        bool hasAttachment = msgObj.contains("hasAttachment") && msgObj["hasAttachment"].toBool();

        QString messageStart;
        if (isSelf) {
            // Własne wiadomości - zielone
            messageStart = QString("%1 <span style=\"color:#60ff8a;\">[You]:</span> ").arg(timestamp);
        } else {
            // Cudze wiadomości - niebieskie
            messageStart = QString("%1 <span style=\"color:#85c4ff;\">[%2]:</span> ").arg(timestamp, senderName);
        }

        if (hasAttachment) {
            qDebug() << "Załącznik wykryty w wiadomości";
            QString attachmentType = msgObj["attachmentType"].toString();
            QString attachmentData = msgObj["attachmentData"].toString();
            QString attachmentName = msgObj["attachmentName"].toString();
            QString attachmentMimeType = msgObj["attachmentMimeType"].toString();

            qDebug() << "Attachment details:" << attachmentType << attachmentName << attachmentMimeType;
            qDebug() << "Attachment data length:" << attachmentData.length();

            // Generujemy unikalny identyfikator dla elementu
            QString elementId = "attachment_" + QUuid::createUuid().toString(QUuid::WithoutBraces);

            if (attachmentType == "image" && !attachmentData.isEmpty()) {
                // Sprawdź, czy to GIF
                if (attachmentMimeType == "image/gif") {
                    qDebug() << "Formatowanie GIF:" << attachmentName;

                    // Renderowanie GIF - specjalny format dla naszej obsługi
                    formattedMsg = messageStart + "<br>" +
                                   QString("<div class='gif-placeholder' "
                                          "data-gif-id='%1' "
                                          "data-mime-type='%2' "
                                          "data-base64='%3' "
                                          "data-filename='%4'>"
                                          "</div>")
                                   .arg(elementId, attachmentMimeType, attachmentData, attachmentName);
                } else {
                    // Obrazy statyczne - używamy własnego formatu dla naszego dekodera obrazów
                    qDebug() << "Formatowanie obrazu:" << attachmentName;
                    qDebug() << "MIME:" << attachmentMimeType;
                    qDebug() << "Długość danych base64:" << attachmentData.length() << "bajtów";

                    // Zmiana na format placeholder podobny do innych mediów
                    formattedMsg = messageStart + "<br>" +
                                  QString("<div class='image-placeholder' "
                                         "data-image-id='%1' "
                                         "data-mime-type='%2' "
                                         "data-base64='%3' "
                                         "data-filename='%4'>"
                                         "</div>")
                                  .arg(elementId, attachmentMimeType, attachmentData, attachmentName);
                }
            }
            else if (attachmentType == "audio" && !attachmentData.isEmpty()) {
                // Renderowanie audio - specjalny format dla naszej obsługi (podobnie jak wideo)
                formattedMsg = messageStart + "<br>" +
                               QString("<div class='audio-placeholder' "
                                      "data-audio-id='%1' "
                                      "data-mime-type='%2' "
                                      "data-base64='%3' "
                                      "data-filename='%4'>"
                                      "</div>")
                               .arg(elementId, attachmentMimeType, attachmentData, attachmentName);
            }
            else if (attachmentType == "video" && !attachmentData.isEmpty()) {
                // Renderowanie wideo - specjalny format dla naszej obsługi
                formattedMsg = messageStart + "<br>" +
                               QString("<div class='video-placeholder' "
                                      "data-video-id='%1' "
                                      "data-mime-type='%2' "
                                      "data-base64='%3' "
                                      "data-filename='%4'>"
                                      "</div>")
                               .arg(elementId, attachmentMimeType, attachmentData, attachmentName);
            }
            else {
                // Inne typy plików - renderuj jako link do pobrania
                formattedMsg = messageStart + " " +
                               QString("<a href='file:%1' style='color:#85c4ff;'>[%2]</a> (%3)")
                               .arg(elementId, attachmentName, MessageFormatter::formatFileSize(attachmentData.length()));
            }
        } else {
            // Zwykła wiadomość tekstowa
            if (content.isEmpty()) {
                content = "[Empty message]";
            }

            // Uwzględnij nowe linie w treści
            content.replace("\n", "<br>");

            if (isSelf) {
                formattedMsg = messageStart + "<span style=\"color:#ffffff;\">" + content + "</span>";
            } else {
                formattedMsg = messageStart + "<span style=\"color:#ffffff;\">" + content + "</span>";
            }
        }

        return formattedMsg;
    }

    static QString formatFileSize(qint64 sizeInBytes) {
        if (sizeInBytes < 1024)
            return QString("%1 bytes").arg(sizeInBytes);
        else if (sizeInBytes < 1024 * 1024)
            return QString("%1 KB").arg(sizeInBytes / 1024.0, 0, 'f', 1);
        else if (sizeInBytes < 1024 * 1024 * 1024)
            return QString("%1 MB").arg(sizeInBytes / (1024.0 * 1024.0), 0, 'f', 2);
        else
            return QString("%1 GB").arg(sizeInBytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    }

    // Formatowanie komunikatów systemowych
    static QString formatSystemMessage(const QString& message) {
        QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
        return QString("%1 <span style=\"color:#ffcc00;\">%2</span>").arg(timestamp, message);
    }
};

#endif // MESSAGE_FORMATTER_H