#include "wavelength_leaver.h"

void WavelengthLeaver::LeaveWavelength() {
    WavelengthRegistry* registry = WavelengthRegistry::GetInstance();
    QString frequency = registry->GetActiveWavelength();

    if (frequency == "-1") {
        qDebug() << "No active wavelength to leave";
        return;
    }

    if (!registry->HasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "not found in registry";
        return;
    }

    const WavelengthInfo info = registry->GetWavelengthInfo(frequency);

    if (info.socket) {
        if (info.socket->isValid()) {
            if (info.is_host) {
                qDebug() << "Closing wavelength as host";
                MessageHandler::GetInstance()->SendSystemCommand(info.socket, "close_wavelength",
                                                                 {{"frequency", frequency}});
            } else {
                qDebug() << "Leaving wavelength as client";
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
    WavelengthRegistry* registry = WavelengthRegistry::GetInstance();

    if (!registry->HasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "not found in registry";
        return;
    }

    const WavelengthInfo info = registry->GetWavelengthInfo(frequency);

    if (!info.is_host) {
        qDebug() << "Cannot close wavelength" << frequency << "- not the host";
        return;
    }

    // Mark as closing to prevent disconnect handler from triggering again
    registry->MarkWavelengthClosing(frequency, true);

    const QString active_frequency = registry->GetActiveWavelength();
    const bool was_active = active_frequency == frequency;

    QWebSocket* socket_to_close = info.socket;

    if (socket_to_close && socket_to_close->isValid()) {
        qDebug() << "Sending close command to server for wavelength" << frequency;
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
