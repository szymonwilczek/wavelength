#ifndef MESSAGE_FORMATTER_H
#define MESSAGE_FORMATTER_H

#include <QJsonObject>
#include "../registry/wavelength_registry.h"

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
            QString timeStr = msgObj["timestamp"].toString();
            if (timeStr.contains("T")) {
                // Format ISO
                msgTime = QDateTime::fromString(timeStr, Qt::ISODate);
            } else {
                // Unix timestamp
                msgTime = QDateTime::fromMSecsSinceEpoch(msgObj["timestamp"].toVariant().toLongLong());
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
        
        if (isSelf) {
            // Własne wiadomości - zielone
            formattedMsg = QString("%1 <span style=\"color:#60ff8a;\">[You]:</span> %2").arg(timestamp, content);
        } else {
            // Cudze wiadomości - niebieskie
            formattedMsg = QString("%1 <span style=\"color:#85c4ff;\">[%2]:</span> %3").arg(timestamp, senderName, content);
        }
        
        return formattedMsg;
    }

    // Formatowanie komunikatów systemowych
    static QString formatSystemMessage(const QString& message) {
        QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
        return QString("%1 <span style=\"color:#ffcc00;\">%2</span>").arg(timestamp, message);
    }
};

#endif // MESSAGE_FORMATTER_H