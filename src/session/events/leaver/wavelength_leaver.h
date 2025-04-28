#ifndef WAVELENGTH_LEAVER_H
#define WAVELENGTH_LEAVER_H

#include <QObject>
#include <QString>

#include "../../../storage/wavelength_registry.h"
#include "../../../storage/database_manager.h"
#include "../../../chat/messages/handler/message_handler.h"

class WavelengthLeaver final : public QObject {
    Q_OBJECT

public:
    static WavelengthLeaver* getInstance() {
        static WavelengthLeaver instance;
        return &instance;
    }

    void leaveWavelength();

    void closeWavelength(QString frequency);

signals:
    void wavelengthLeft(QString frequency);
    void wavelengthClosed(QString frequency);

private:
    explicit WavelengthLeaver(QObject* parent = nullptr) : QObject(parent) {}
    ~WavelengthLeaver() override {}

    WavelengthLeaver(const WavelengthLeaver&) = delete;
    WavelengthLeaver& operator=(const WavelengthLeaver&) = delete;
};

#endif // WAVELENGTH_LEAVER_H