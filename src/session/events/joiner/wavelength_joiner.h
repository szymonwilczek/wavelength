#ifndef WAVELENGTH_JOINER_H
#define WAVELENGTH_JOINER_H

#include <QObject>
#include <QString>
#include <QTimer>

#include "../../../storage/database_manager.h"
#include "../../../storage/wavelength_registry.h"
#include "../../../auth/authentication_manager.h"
#include "../../../chat/messages/services/wavelength_message_processor.h"
#include "../../../chat/messages/formatter/message_formatter.h"
#include "../../../chat/messages/handler/message_handler.h"
#include "../../../../src/app/wavelength_config.h"

struct JoinResult {
    bool success;
    QString errorReason;
};

class WavelengthJoiner : public QObject {
    Q_OBJECT

public:
    static WavelengthJoiner* getInstance() {
        static WavelengthJoiner instance;
        return &instance;
    }

    JoinResult joinWavelength(QString frequency, const QString& password = QString());

signals:
    void wavelengthJoined(QString frequency);
    void wavelengthClosed(QString frequency);
    void authenticationFailed(QString frequency);
    void connectionError(const QString& errorMessage);
    void messageReceived(QString frequency, const QString& formattedMessage);
    void wavelengthLeft(QString frequency);

private:
    WavelengthJoiner(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthJoiner() {}

    WavelengthJoiner(const WavelengthJoiner&) = delete;
    WavelengthJoiner& operator=(const WavelengthJoiner&) = delete;
};

#endif // WAVELENGTH_JOINER_H