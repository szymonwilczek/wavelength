//
// Created by szymo on 28.04.2025.
//

#include "attachment_data_store.h"

QString AttachmentDataStore::storeAttachmentData(const QString &base64Data) {
    QMutexLocker locker(&m_mutex);
    QString attachmentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_attachmentData[attachmentId] = base64Data;
    return attachmentId;
}

QString AttachmentDataStore::getAttachmentData(const QString &attachmentId) {
    QMutexLocker locker(&m_mutex);
    if (m_attachmentData.contains(attachmentId)) {
        return m_attachmentData[attachmentId];
    }
    return QString();
}

void AttachmentDataStore::removeAttachmentData(const QString &attachmentId) {
    QMutexLocker locker(&m_mutex);
    m_attachmentData.remove(attachmentId);
}
