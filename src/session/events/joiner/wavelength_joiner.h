#ifndef WAVELENGTH_JOINER_H
#define WAVELENGTH_JOINER_H

#include "../../../storage/database_manager.h"
#include "../../../storage/wavelength_registry.h"
#include "../../../chat/messages/services/wavelength_message_processor.h"
#include "../../../../src/app/wavelength_config.h"

struct JoinResult {
    bool success;
    QString error_reason;
};

class WavelengthJoiner final : public QObject {
    Q_OBJECT

public:
    static WavelengthJoiner* GetInstance() {
        static WavelengthJoiner instance;
        return &instance;
    }

    JoinResult JoinWavelength(QString frequency, const QString& password = QString());

signals:
    void wavelengthJoined(QString frequency);
    void wavelengthClosed(QString frequency);
    void authenticationFailed(QString frequency);
    void connectionError(const QString& error_message);
    void messageReceived(QString frequency, const QString& formatted_message);
    void wavelengthLeft(QString frequency);

private:
    explicit WavelengthJoiner(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthJoiner() override {}

    WavelengthJoiner(const WavelengthJoiner&) = delete;
    WavelengthJoiner& operator=(const WavelengthJoiner&) = delete;
};

#endif // WAVELENGTH_JOINER_H