#ifndef WAVELENGTH_CREATOR_H
#define WAVELENGTH_CREATOR_H

#include "../../../storage/database_manager.h"
#include "../../../chat/messages/services/wavelength_message_processor.h"
#include "../../../../src/app/wavelength_config.h"

class WavelengthCreator : public QObject {
    Q_OBJECT

public:
    static WavelengthCreator* getInstance() {
        static WavelengthCreator instance;
        return &instance;
    }

    bool createWavelength(QString frequency, bool isPasswordProtected,
                  const QString& password);

signals:
    void wavelengthCreated(QString frequency);
    void wavelengthClosed(QString frequency);
    void connectionError(const QString& errorMessage);

private:
    WavelengthCreator(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthCreator() {}

    WavelengthCreator(const WavelengthCreator&) = delete;
    WavelengthCreator& operator=(const WavelengthCreator&) = delete;
};

#endif // WAVELENGTH_CREATOR_H