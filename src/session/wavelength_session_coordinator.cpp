#include "wavelength_session_coordinator.h"

#include "events/creator/wavelength_creator.h"
#include "events/joiner/wavelength_joiner.h"
#include "events/leaver/wavelength_leaver.h"

void WavelengthSessionCoordinator::Initialize() {
    ConnectSignals();
    LoadConfig();
}

bool WavelengthSessionCoordinator::CreateWavelength(const QString &frequency, const bool is_password_protected,
    const QString &password) {
    const bool success = WavelengthCreator::GetInstance()->CreateWavelength(
        frequency, is_password_protected, password);

    if (success) {
        WavelengthStateManager::GetInstance()->RegisterJoinedWavelength(frequency);
    }

    return success;
}

bool WavelengthSessionCoordinator::JoinWavelength(const QString &frequency, const QString &password) {
    const auto [success, errorReason] = WavelengthJoiner::GetInstance()->JoinWavelength(frequency, password);

    if (success) {
        WavelengthStateManager::GetInstance()->RegisterJoinedWavelength(frequency);
    }

    return success;
}

void WavelengthSessionCoordinator::LeaveWavelength() {
    const QString frequency = WavelengthStateManager::GetInstance()->GetActiveWavelength();

    if (frequency != -1) {
        WavelengthStateManager::GetInstance()->UnregisterJoinedWavelength(frequency);
    }

    WavelengthLeaver::GetInstance()->LeaveWavelength();
}

void WavelengthSessionCoordinator::CloseWavelength(const QString &frequency) {
    WavelengthStateManager::GetInstance()->UnregisterJoinedWavelength(frequency);
    WavelengthLeaver::GetInstance()->CloseWavelength(frequency);
}

bool WavelengthSessionCoordinator::SendMessage(const QString &message) {
    return WavelengthMessageService::GetInstance()->SendTextMessage(message);
}

bool WavelengthSessionCoordinator::IsWavelengthJoined(const QString &frequency) {
    return WavelengthStateManager::GetInstance()->IsWavelengthJoined(frequency);
}

bool WavelengthSessionCoordinator::IsWavelengthConnected(const QString &frequency) {
    return WavelengthStateManager::GetInstance()->IsWavelengthConnected(frequency);
}

void WavelengthSessionCoordinator::ConnectSignals() {
    // BEZPOŚREDNIE POŁĄCZENIA Z SYGNAŁAMI KOMPONENTÓW

    // WavelengthCreator
    connect(WavelengthCreator::GetInstance(), &WavelengthCreator::wavelengthCreated,
            this, &WavelengthSessionCoordinator::onWavelengthCreated, Qt::DirectConnection);
    connect(WavelengthCreator::GetInstance(), &WavelengthCreator::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
    connect(WavelengthCreator::GetInstance(), &WavelengthCreator::connectionError,
            this, &WavelengthSessionCoordinator::onConnectionError, Qt::DirectConnection);

    // WavelengthJoiner
    connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::wavelengthJoined,
            this, &WavelengthSessionCoordinator::onWavelengthJoined, Qt::DirectConnection);
    connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
    connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::connectionError,
            this, &WavelengthSessionCoordinator::onConnectionError, Qt::DirectConnection);
    connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::authenticationFailed,
            this, &WavelengthSessionCoordinator::onAuthenticationFailed, Qt::DirectConnection);
    connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::messageReceived,
            this, &WavelengthSessionCoordinator::onMessageReceived, Qt::DirectConnection);

    // WavelengthLeaver
    connect(WavelengthLeaver::GetInstance(), &WavelengthLeaver::wavelengthLeft,
            this, &WavelengthSessionCoordinator::onWavelengthLeft, Qt::DirectConnection);
    connect(WavelengthLeaver::GetInstance(), &WavelengthLeaver::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);

    // WavelengthMessageService
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::messageSent,
            this, &WavelengthSessionCoordinator::onMessageSent, Qt::DirectConnection);
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::pttGranted, this, &WavelengthSessionCoordinator::onPttGranted);
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::pttDenied, this, &WavelengthSessionCoordinator::onPttDenied);
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::pttStartReceiving, this, &WavelengthSessionCoordinator::onPttStartReceiving);
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::pttStopReceiving, this, &WavelengthSessionCoordinator::onPttStopReceiving);
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::audioDataReceived, this, &WavelengthSessionCoordinator::onAudioDataReceived);
    connect(WavelengthMessageService::GetInstance(), &WavelengthMessageService::remoteAudioAmplitudeUpdate, this, &WavelengthSessionCoordinator::onRemoteAudioAmplitudeUpdate);

    // WavelengthMessageProcessor
    connect(WavelengthMessageProcessor::GetInstance(), &WavelengthMessageProcessor::messageReceived,
            this, &WavelengthSessionCoordinator::onMessageReceived, Qt::DirectConnection);
    connect(WavelengthMessageProcessor::GetInstance(), &WavelengthMessageProcessor::wavelengthClosed,
            this, &WavelengthSessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
    connect(WavelengthMessageProcessor::GetInstance(), &WavelengthMessageProcessor::userKicked,
            this, &WavelengthSessionCoordinator::userKicked, Qt::DirectConnection);

    // WavelengthStateManager
    connect(WavelengthStateManager::GetInstance(), &WavelengthStateManager::activeWavelengthChanged,
            this, &WavelengthSessionCoordinator::onActiveWavelengthChanged, Qt::DirectConnection);

    // WavelengthConfig
    connect(WavelengthConfig::GetInstance(), &WavelengthConfig::configChanged,
            this, &WavelengthSessionCoordinator::onConfigChanged, Qt::DirectConnection);
}

void WavelengthSessionCoordinator::LoadConfig() {
    WavelengthConfig* config = WavelengthConfig::GetInstance();

    if (!QFile::exists(config->GetSetting("configFilePath").toString())) {

        config->SetRelayServerAddress("localhost");
        config->SetRelayServerPort(3000);

        // Zapisz konfigurację
        config->SaveSettings();
    }
}
