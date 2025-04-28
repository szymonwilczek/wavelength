#ifndef MESSAGE_FORMATTER_H
#define MESSAGE_FORMATTER_H

#include <QJsonObject>
#include <QUuid>
#include <QDebug>
#include <QDateTime>
#include <QMutex>
#include <QMap>
#include "../../../storage/wavelength_registry.h"
#include "../../files/attachments/attachment_data_store.h"


class MessageFormatter {
public:
    static QString formatMessage(const QJsonObject& msgObj, QString frequency);

    static QString formatFileSize(qint64 sizeInBytes);

    static QString formatSystemMessage(const QString& message);
};

#endif // MESSAGE_FORMATTER_H