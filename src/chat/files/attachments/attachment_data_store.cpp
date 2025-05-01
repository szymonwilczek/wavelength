#include "attachment_data_store.h"

#include <QUuid>

QString AttachmentDataStore::StoreAttachmentData(const QString &base64_data) {
    QMutexLocker locker(&mutex_);
    QString attachment_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    attachment_data_[attachment_id] = base64_data;
    return attachment_id;
}

QString AttachmentDataStore::GetAttachmentData(const QString &attachment_id) {
    QMutexLocker locker(&mutex_);
    if (attachment_data_.contains(attachment_id)) {
        return attachment_data_[attachment_id];
    }
    return QString();
}

void AttachmentDataStore::RemoveAttachmentData(const QString &attachment_id) {
    QMutexLocker locker(&mutex_);
    attachment_data_.remove(attachment_id);
}
