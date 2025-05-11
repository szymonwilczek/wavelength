#include "wavelength_leaver.h"

#include "../../../chat/messages/handler/message_handler.h"
#include "../../../storage/wavelength_registry.h"

void WavelengthLeaver::LeaveWavelength() {
    WavelengthRegistry *registry = WavelengthRegistry::GetInstance();
    QString frequency = registry->GetActiveWavelength();

    if (frequency == "-1") {
        qDebug() << "[LEAVER] No active wavelength to leave";
        return;
    }

    if (!registry->HasWavelength(frequency)) {
        qDebug() << "[LEAVER] Wavelength" << frequency << "not found in registry";
        return;
    }

    const WavelengthInfo info = registry->GetWavelengthInfo(frequency);

    if (info.socket) {
        if (info.socket->isValid()) {
            if (info.is_host) {
                MessageHandler::GetInstance()->SendSystemCommand(info.socket, "close_wavelength",
                                                                 {{"frequency", frequency}});
            } else {
                MessageHandler::GetInstance()->SendSystemCommand(info.socket, "leave_wavelength",
                                                                 {{"frequency", frequency}});
            }

            info.socket->close();
        }
    }

    registry->RemoveWavelength(frequency);


    registry->SetActiveWavelength("-1");
    emit wavelengthLeft(frequency);
}

void WavelengthLeaver::CloseWavelength(QString frequency) {
    WavelengthRegistry *registry = WavelengthRegistry::GetInstance();

    if (!registry->HasWavelength(frequency)) {
        qDebug() << "[LEAVER] Wavelength" << frequency << "not found in registry";
        return;
    }

    const WavelengthInfo info = registry->GetWavelengthInfo(frequency);

    if (!info.is_host) {
        qDebug() << "[LEAVER] Cannot close wavelength" << frequency << "- not the host";
        return;
    }

    registry->MarkWavelengthClosing(frequency, true);

    const QString active_frequency = registry->GetActiveWavelength();
    const bool was_active = active_frequency == frequency;

    QWebSocket *socket_to_close = info.socket;

    if (socket_to_close && socket_to_close->isValid()) {
        MessageHandler::GetInstance()->SendSystemCommand(socket_to_close, "close_wavelength",
                                                         {{"frequency", frequency}});

        socket_to_close->close();
    } else if (socket_to_close) {
        socket_to_close->deleteLater();
    }

    registry->RemoveWavelength(frequency);


    if (was_active) {
        registry->SetActiveWavelength("-1");
    }

    emit wavelengthClosed(frequency);
}
