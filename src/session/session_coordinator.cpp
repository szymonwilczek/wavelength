#include "session_coordinator.h"

#include "events/creator/wavelength_creator.h"
#include "events/joiner/wavelength_joiner.h"
#include "events/leaver/wavelength_leaver.h"

void SessionCoordinator::Initialize() {
        ConnectSignals();
        LoadConfig();
}

bool SessionCoordinator::CreateWavelength(const QString &frequency, const bool is_password_protected,
                                                    const QString &password) {
        const bool success = WavelengthCreator::GetInstance()->CreateWavelength(
                frequency, is_password_protected, password);

        if (success) {
                WavelengthStateManager::GetInstance()->RegisterJoinedWavelength(frequency);
        }

        return success;
}

bool SessionCoordinator::JoinWavelength(const QString &frequency, const QString &password) {
        const auto [success, errorReason] = WavelengthJoiner::GetInstance()->JoinWavelength(frequency, password);

        if (success) {
                WavelengthStateManager::GetInstance()->RegisterJoinedWavelength(frequency);
        }

        return success;
}

void SessionCoordinator::LeaveWavelength() {
        const QString frequency = WavelengthStateManager::GetInstance()->GetActiveWavelength();

        if (frequency != -1) {
                WavelengthStateManager::GetInstance()->UnregisterJoinedWavelength(frequency);
        }

        WavelengthLeaver::GetInstance()->LeaveWavelength();
}

void SessionCoordinator::CloseWavelength(const QString &frequency) {
        WavelengthStateManager::GetInstance()->UnregisterJoinedWavelength(frequency);
        WavelengthLeaver::GetInstance()->CloseWavelength(frequency);
}

bool SessionCoordinator::SendMessage(const QString &message) {
        return MessageService::GetInstance()->SendTextMessage(message);
}

bool SessionCoordinator::IsWavelengthJoined(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthJoined(frequency);
}

bool SessionCoordinator::IsWavelengthConnected(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthConnected(frequency);
}

void SessionCoordinator::ConnectSignals() {
        // WavelengthCreator
        connect(WavelengthCreator::GetInstance(), &WavelengthCreator::wavelengthCreated,
                this, &SessionCoordinator::onWavelengthCreated, Qt::DirectConnection);
        connect(WavelengthCreator::GetInstance(), &WavelengthCreator::wavelengthClosed,
                this, &SessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
        connect(WavelengthCreator::GetInstance(), &WavelengthCreator::connectionError,
                this, &SessionCoordinator::onConnectionError, Qt::DirectConnection);

        // WavelengthJoiner
        connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::wavelengthJoined,
                this, &SessionCoordinator::onWavelengthJoined, Qt::DirectConnection);
        connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::wavelengthClosed,
                this, &SessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
        connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::connectionError,
                this, &SessionCoordinator::onConnectionError, Qt::DirectConnection);
        connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::authenticationFailed,
                this, &SessionCoordinator::onAuthenticationFailed, Qt::DirectConnection);
        connect(WavelengthJoiner::GetInstance(), &WavelengthJoiner::messageReceived,
                this, &SessionCoordinator::onMessageReceived, Qt::DirectConnection);

        // WavelengthLeaver
        connect(WavelengthLeaver::GetInstance(), &WavelengthLeaver::wavelengthLeft,
                this, &SessionCoordinator::onWavelengthLeft, Qt::DirectConnection);
        connect(WavelengthLeaver::GetInstance(), &WavelengthLeaver::wavelengthClosed,
                this, &SessionCoordinator::onWavelengthClosed, Qt::DirectConnection);

        // WavelengthMessageService
        connect(MessageService::GetInstance(), &MessageService::messageSent,
                this, &SessionCoordinator::onMessageSent, Qt::DirectConnection);
        connect(MessageService::GetInstance(), &MessageService::pttGranted, this,
                &SessionCoordinator::onPttGranted);
        connect(MessageService::GetInstance(), &MessageService::pttDenied, this,
                &SessionCoordinator::onPttDenied);
        connect(MessageService::GetInstance(), &MessageService::pttStartReceiving, this,
                &SessionCoordinator::onPttStartReceiving);
        connect(MessageService::GetInstance(), &MessageService::pttStopReceiving, this,
                &SessionCoordinator::onPttStopReceiving);
        connect(MessageService::GetInstance(), &MessageService::audioDataReceived, this,
                &SessionCoordinator::onAudioDataReceived);
        connect(MessageService::GetInstance(), &MessageService::remoteAudioAmplitudeUpdate, this,
                &SessionCoordinator::onRemoteAudioAmplitudeUpdate);

        // WavelengthMessageProcessor
        connect(MessageProcessor::GetInstance(), &MessageProcessor::messageReceived,
                this, &SessionCoordinator::onMessageReceived, Qt::DirectConnection);
        connect(MessageProcessor::GetInstance(), &MessageProcessor::wavelengthClosed,
                this, &SessionCoordinator::onWavelengthClosed, Qt::DirectConnection);
        connect(MessageProcessor::GetInstance(), &MessageProcessor::userKicked,
                this, &SessionCoordinator::userKicked, Qt::DirectConnection);

        // WavelengthStateManager
        connect(WavelengthStateManager::GetInstance(), &WavelengthStateManager::activeWavelengthChanged,
                this, &SessionCoordinator::onActiveWavelengthChanged, Qt::DirectConnection);

        // WavelengthConfig
        connect(WavelengthConfig::GetInstance(), &WavelengthConfig::configChanged,
                this, &SessionCoordinator::onConfigChanged, Qt::DirectConnection);
}

void SessionCoordinator::LoadConfig() {
        WavelengthConfig *config = WavelengthConfig::GetInstance();

        if (!QFile::exists(config->GetSetting("configFilePath").toString())) {
                config->SetRelayServerAddress("localhost");
                config->SetRelayServerPort(3000);
                config->SaveSettings();
        }
}
