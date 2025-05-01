#include "wavelength_session_coordinator.h"

#include "events/creator/wavelength_creator.h"
#include "events/leaver/wavelength_leaver.h"

void WavelengthSessionCoordinator::initialize() {
    qDebug() << "Initializing WavelengthSessionCoordinator";

    // Połącz sygnały między komponentami
    connectSignals();

    // Załaduj konfigurację
    loadConfig();

    qDebug() << "WavelengthSessionCoordinator initialized successfully";
}

bool WavelengthSessionCoordinator::createWavelength(const QString &frequency, const bool isPasswordProtected,
    const QString &password) {
    qDebug() << "Coordinator: Creating wavelength" << frequency;
    const bool success = WavelengthCreator::getInstance()->createWavelength(
        frequency, isPasswordProtected, password);

    if (success) {
        WavelengthStateManager::getInstance()->registerJoinedWavelength(frequency);
    }

    return success;
}

bool WavelengthSessionCoordinator::joinWavelength(const QString &frequency, const QString &password) {
    qDebug() << "Coordinator: Joining wavelength" << frequency;
    const auto result = WavelengthJoiner::getInstance()->joinWavelength(frequency, password);

    if (result.success) {
        WavelengthStateManager::getInstance()->registerJoinedWavelength(frequency);
    }

    return result.success;
}

void WavelengthSessionCoordinator::leaveWavelength() {
    qDebug() << "Coordinator: Leaving active wavelength";
    const QString frequency = WavelengthStateManager::getInstance()->getActiveWavelength();

    if (frequency != -1) {
        WavelengthStateManager::getInstance()->unregisterJoinedWavelength(frequency);
    }

    WavelengthLeaver::getInstance()->leaveWavelength();
}

void WavelengthSessionCoordinator::closeWavelength(const QString &frequency) {
    qDebug() << "Coordinator: Closing wavelength" << frequency;
    WavelengthStateManager::getInstance()->unregisterJoinedWavelength(frequency);
    WavelengthLeaver::getInstance()->closeWavelength(frequency);
}

bool WavelengthSessionCoordinator::sendMessage(const QString &message) {
    qDebug() << "Coordinator: Sending message to active wavelength";
    return WavelengthMessageService::getInstance()->sendMessage(message);
}

bool WavelengthSessionCoordinator::isWavelengthJoined(const QString &frequency) {
    return WavelengthStateManager::getInstance()->isWavelengthJoined(frequency);
}

bool WavelengthSessionCoordinator::isWavelengthConnected(const QString &frequency) {
    return WavelengthStateManager::getInstance()->isWavelengthConnected(frequency);
}

void WavelengthSessionCoordinator::connectSignals() {
    // BEZPOŚREDNIE POŁĄCZENIA Z SYGNAŁAMI KOMPONENTÓW

    // WavelengthCreator
    connect(WavelengthCreator::getInstance(), &WavelengthCreator::wavelengthCreated,
            this, &WavelengthSessionCoordinator::onWavelengthCreated, Qt::DirectConnection);
    connect(WavelengthCreator::getInstance(), &WavelengthCreator::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
    connect(WavelengthCreator::getInstance(), &WavelengthCreator::connectionError,
            this, &WavelengthSessionCoordinator::onConnectionError, Qt::DirectConnection);

    // WavelengthJoiner
    connect(WavelengthJoiner::getInstance(), &WavelengthJoiner::wavelengthJoined,
            this, &WavelengthSessionCoordinator::onWavelengthJoined, Qt::DirectConnection);
    connect(WavelengthJoiner::getInstance(), &WavelengthJoiner::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
    connect(WavelengthJoiner::getInstance(), &WavelengthJoiner::connectionError,
            this, &WavelengthSessionCoordinator::onConnectionError, Qt::DirectConnection);
    connect(WavelengthJoiner::getInstance(), &WavelengthJoiner::authenticationFailed,
            this, &WavelengthSessionCoordinator::onAuthenticationFailed, Qt::DirectConnection);
    connect(WavelengthJoiner::getInstance(), &WavelengthJoiner::messageReceived,
            this, &WavelengthSessionCoordinator::onMessageReceived, Qt::DirectConnection);

    // WavelengthLeaver
    connect(WavelengthLeaver::getInstance(), &WavelengthLeaver::wavelengthLeft,
            this, &WavelengthSessionCoordinator::onWavelengthLeft, Qt::DirectConnection);
    connect(WavelengthLeaver::getInstance(), &WavelengthLeaver::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);

    // WavelengthMessageService
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::messageSent,
            this, &WavelengthSessionCoordinator::onMessageSent, Qt::DirectConnection);
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::pttGranted, this, &WavelengthSessionCoordinator::onPttGranted);
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::pttDenied, this, &WavelengthSessionCoordinator::onPttDenied);
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::pttStartReceiving, this, &WavelengthSessionCoordinator::onPttStartReceiving);
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::pttStopReceiving, this, &WavelengthSessionCoordinator::onPttStopReceiving);
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::audioDataReceived, this, &WavelengthSessionCoordinator::onAudioDataReceived);
    connect(WavelengthMessageService::getInstance(), &WavelengthMessageService::remoteAudioAmplitudeUpdate, this, &WavelengthSessionCoordinator::onRemoteAudioAmplitudeUpdate);

    // WavelengthMessageProcessor
    connect(WavelengthMessageProcessor::getInstance(), &WavelengthMessageProcessor::messageReceived,
            this, &WavelengthSessionCoordinator::onMessageReceived, Qt::DirectConnection);
    connect(WavelengthMessageProcessor::getInstance(), &WavelengthMessageProcessor::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
    connect(WavelengthMessageProcessor::getInstance(), &WavelengthMessageProcessor::userKicked,
            this, &WavelengthSessionCoordinator::userKicked, Qt::DirectConnection);

    // WavelengthStateManager
    connect(WavelengthStateManager::getInstance(), &WavelengthStateManager::activeWavelengthChanged,
            this, &WavelengthSessionCoordinator::onActiveWavelengthChanged, Qt::DirectConnection);

    // WavelengthConfig
    connect(WavelengthConfig::GetInstance(), &WavelengthConfig::configChanged,
            this, &WavelengthSessionCoordinator::onConfigChanged, Qt::DirectConnection);
}

void WavelengthSessionCoordinator::loadConfig() {
    WavelengthConfig* config = WavelengthConfig::GetInstance();

    if (!QFile::exists(config->GetSetting("configFilePath").toString())) {
        qDebug() << "Setting default configuration";

        config->SetRelayServerAddress("localhost");
        config->SetRelayServerPort(3000);

        // Zapisz konfigurację
        config->SaveSettings();
    }
}
