#include "wavelength_leaver.h"

void WavelengthLeaver::leaveWavelength() {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();
    QString freq = registry->getActiveWavelength();

    if (freq == "-1") {
        qDebug() << "No active wavelength to leave";
        return;
    }

    if (!registry->hasWavelength(freq)) {
        qDebug() << "Wavelength" << freq << "not found in registry";
        return;
    }

    WavelengthInfo info = registry->getWavelengthInfo(freq);

    if (info.socket) {
        if (info.socket->isValid()) {
            if (info.isHost) {
                qDebug() << "Closing wavelength as host";
                MessageHandler::getInstance()->sendSystemCommand(info.socket, "close_wavelength",
                                                                 {{"frequency", freq}});
            } else {
                qDebug() << "Leaving wavelength as client";
                MessageHandler::getInstance()->sendSystemCommand(info.socket, "leave_wavelength",
                                                                 {{"frequency", freq}});
            }

            info.socket->close();
        }
    }

    registry->removeWavelength(freq);


    registry->setActiveWavelength("-1");
    emit wavelengthLeft(freq);
}

void WavelengthLeaver::closeWavelength(QString frequency) {
    WavelengthRegistry* registry = WavelengthRegistry::getInstance();

    if (!registry->hasWavelength(frequency)) {
        qDebug() << "Wavelength" << frequency << "not found in registry";
        return;
    }

    WavelengthInfo info = registry->getWavelengthInfo(frequency);

    if (!info.isHost) {
        qDebug() << "Cannot close wavelength" << frequency << "- not the host";
        return;
    }

    // Mark as closing to prevent disconnect handler from triggering again
    registry->markWavelengthClosing(frequency, true);

    QString activeFreq = registry->getActiveWavelength();
    bool wasActive = (activeFreq == frequency);

    QWebSocket* socketToClose = info.socket;

    if (socketToClose && socketToClose->isValid()) {
        qDebug() << "Sending close command to server for wavelength" << frequency;
        MessageHandler::getInstance()->sendSystemCommand(socketToClose, "close_wavelength",
                                                         {{"frequency", frequency}});

        socketToClose->close();
    } else if (socketToClose) {
        socketToClose->deleteLater();
    }

    registry->removeWavelength(frequency);


    if (wasActive) {
        registry->setActiveWavelength("-1");
    }

    emit wavelengthClosed(frequency);
}
