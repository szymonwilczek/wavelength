#ifndef ATTACHMENT_DATA_STORE_H
#define ATTACHMENT_DATA_STORE_H
#include <QMap>
#include <qmutex.h>
#include <QString>

class AttachmentDataStore {
public:
    static AttachmentDataStore* getInstance() {
        static AttachmentDataStore instance;
        return &instance;
    }

    // Dodaj dane załącznika i zwróć identyfikator
    QString storeAttachmentData(const QString& base64Data);

    // Pobierz dane załącznika
    QString getAttachmentData(const QString& attachmentId);

    // Usuń dane załącznika (do zwalniania pamięci)
    void removeAttachmentData(const QString& attachmentId);

private:
    AttachmentDataStore() = default;
    QMap<QString, QString> m_attachmentData;
    QMutex m_mutex;
};



#endif //ATTACHMENT_DATA_STORE_H
