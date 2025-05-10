#include "session_coordinator.h"

#include <QFile>

#include "../app/wavelength_config.h"
#include "../chat/messages/services/message_processor.h"
#include "../chat/messages/services/message_service.h"
#include "../services/wavelength_event_broker.h"
#include "../services/wavelength_state_manager.h"
#include "../storage/wavelength_registry.h"
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

bool SessionCoordinator::SendFile(const QString &file_path) {
        return MessageService::GetInstance()->SendFile(file_path);
}

WavelengthInfo SessionCoordinator::GetWavelengthInfo(const QString &frequency, bool *is_host) {
        return WavelengthStateManager::GetWavelengthInfo(frequency, is_host);
}

QString SessionCoordinator::GetActiveWavelength() {
        return WavelengthStateManager::GetActiveWavelength();
}

bool SessionCoordinator::IsActiveWavelengthHost() {
        return WavelengthStateManager::IsActiveWavelengthHost();
}

QList<QString> SessionCoordinator::GetJoinedWavelengths() {
        return WavelengthStateManager::GetInstance()->GetJoinedWavelengths();
}

int SessionCoordinator::GetJoinedWavelengthCount() {
        return WavelengthStateManager::GetInstance()->GetJoinedWavelengthCount();
}

bool SessionCoordinator::IsWavelengthPasswordProtected(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthPasswordProtected(frequency);
}

bool SessionCoordinator::IsWavelengthHost(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthHost(frequency);
}

void SessionCoordinator::SetActiveWavelength(const QString &frequency) {
        WavelengthStateManager::GetInstance()->SetActiveWavelength(frequency);
}

bool SessionCoordinator::IsWavelengthJoined(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthJoined(frequency);
}

bool SessionCoordinator::IsWavelengthConnected(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthConnected(frequency);
}

QString SessionCoordinator::GetRelayServerAddress() {
        return WavelengthConfig::GetInstance()->GetRelayServerAddress();
}

void SessionCoordinator::SetRelayServerAddress(const QString &address) {
        WavelengthConfig::GetInstance()->SetRelayServerAddress(address);
}

QString SessionCoordinator::GetRelayServerUrl() {
        return WavelengthConfig::GetInstance()->GetRelayServerUrl();
}

void SessionCoordinator::onWavelengthCreated(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthCreated signal for frequency" << frequency;
        emit wavelengthCreated(frequency);
        WavelengthEventBroker::GetInstance()->WavelengthCreated(frequency);
}

void SessionCoordinator::onWavelengthJoined(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthJoined signal for frequency" << frequency;
        emit wavelengthJoined(frequency);
        WavelengthEventBroker::GetInstance()->WavelengthJoined(frequency);
}

void SessionCoordinator::onWavelengthLeft(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthLeft signal for frequency" << frequency;
        emit wavelengthLeft(frequency);
        WavelengthEventBroker::GetInstance()->WavelengthLeft(frequency);
}

void SessionCoordinator::onWavelengthClosed(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthClosed signal for frequency" << frequency;
        emit wavelengthClosed(frequency);
        WavelengthEventBroker::GetInstance()->WavelengthClosed(frequency);
}

void SessionCoordinator::onMessageReceived(const QString &frequency, const QString &message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageReceived signal";
        emit messageReceived(frequency, message);
        WavelengthEventBroker::GetInstance()->MessageReceived(frequency, message);
}

void SessionCoordinator::onMessageSent(const QString &frequency, const QString &message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageSent signal";
        emit messageSent(frequency, message);
        WavelengthEventBroker::GetInstance()->MessageSent(frequency, message);
}

void SessionCoordinator::onConnectionError(const QString &errorMessage) {
        qDebug() << "WavelengthSessionCoordinator: Propagating connectionError signal:" << errorMessage;
        emit connectionError(errorMessage);
        WavelengthEventBroker::GetInstance()->ConnectionError(errorMessage);
}

void SessionCoordinator::onAuthenticationFailed(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating authenticationFailed signal for frequency" << frequency;
        emit authenticationFailed(frequency);
        WavelengthEventBroker::GetInstance()->AuthenticationFailed(frequency);
}

void SessionCoordinator::onActiveWavelengthChanged(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating activeWavelengthChanged signal for frequency" <<
                        frequency;
        emit activeWavelengthChanged(frequency);
        WavelengthEventBroker::GetInstance()->ActiveWavelengthChanged(frequency);
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
