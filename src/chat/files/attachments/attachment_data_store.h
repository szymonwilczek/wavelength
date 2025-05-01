#ifndef ATTACHMENT_DATA_STORE_H
#define ATTACHMENT_DATA_STORE_H
#include <QMap>
#include <qmutex.h>
#include <QString>

class AttachmentDataStore {
public:
    static AttachmentDataStore* GetInstance() {
        static AttachmentDataStore instance;
        return &instance;
    }

    // Dodaj dane załącznika i zwróć identyfikator
    QString StoreAttachmentData(const QString& base64_data);

    // Pobierz dane załącznika
    QString GetAttachmentData(const QString& attachment_id);

    // Usuń dane załącznika (do zwalniania pamięci)
    void RemoveAttachmentData(const QString& attachment_id);

private:
    AttachmentDataStore() = default;
    QMap<QString, QString> attachment_data_;
    QMutex mutex_;
};



#endif //ATTACHMENT_DATA_STORE_H
