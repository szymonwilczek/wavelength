#include "wavelength_leaver.h"

void WavelengthLeaver::LeaveWavelength() {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    QString frequency = registry->getActiveWavelength();

    if (frequency == "-1") {
        qDebug() << "No active wavelength to leave";
        return;
    }

    if (!registry->hasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "not found in registry";
        return;
    }

    const WavelengthInfo info = registry->getWavelengthInfo(frequency);

    if (info.socket) {
        if (info.socket->isValid()) {
            if (info.isHost) {
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

    registry->removeWavelength(frequency);


    registry->setActiveWavelength("-1");
    emit wavelengthLeft(frequency);
}

void WavelengthLeaver::CloseWavelength(QString frequency) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();

    if (!registry->hasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "not found in registry";
        return;
    }

    const WavelengthInfo info = registry->getWavelengthInfo(frequency);

    if (!info.isHost) {
        qDebug() << "Cannot close wavelength" << frequency << "- not the host";
        return;
    }

    // Mark as closing to prevent disconnect handler from triggering again
    registry->markWavelengthClosing(frequency, true);

    const QString active_frequency = registry->getActiveWavelength();
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

    registry->removeWavelength(frequency);


    if (was_active) {
        registry->setActiveWavelength("-1");
    }

    emit wavelengthClosed(frequency);
}
