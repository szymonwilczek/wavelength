#ifndef MESSAGE_FORMATTER_H
#define MESSAGE_FORMATTER_H

#include "../../../storage/wavelength_registry.h"
#include "../../files/attachments/attachment_data_store.h"

class MessageFormatter {
public:
    static QString FormatMessage(const QJsonObject& message_object, const QString &frequency);

    static QString FormatFileSize(qint64 size_in_bytes);

    static QString FormatSystemMessage(const QString& message);
};

#endif // MESSAGE_FORMATTER_H