#ifndef WAVELENGTH_CREATOR_H
#define WAVELENGTH_CREATOR_H

#include "../../../storage/database_manager.h"
#include "../../../chat/messages/services/wavelength_message_processor.h"
#include "../../../../src/app/wavelength_config.h"

class WavelengthCreator final : public QObject {
    Q_OBJECT

public:
    static WavelengthCreator* GetInstance() {
        static WavelengthCreator instance;
        return &instance;
    }

    bool CreateWavelength(QString frequency, bool is_password_protected,
                  const QString& password);

signals:
    void wavelengthCreated(QString frequency);
    void wavelengthClosed(QString frequency);
    void connectionError(const QString& error_message);

private:
    explicit WavelengthCreator(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthCreator() override {}

    WavelengthCreator(const WavelengthCreator&) = delete;
    WavelengthCreator& operator=(const WavelengthCreator&) = delete;
};

#endif // WAVELENGTH_CREATOR_H