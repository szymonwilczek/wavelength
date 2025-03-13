#ifndef WAVELENGTH_SESSION_COORDINATOR_H
#define WAVELENGTH_SESSION_COORDINATOR_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <qfile.h>

#include "events/creator/wavelength_creator.h"
#include "events/joiner/wavelength_joiner.h"
#include "events/leaver/wavelength_leaver.h"
#include "../messages/wavelength_message_service.h"
#include "../messages/wavelength_message_processor.h"
#include "../managers/wavelength_state_manager.h"
#include "../socket/wavelength_socket_handler.h"
#include "../managers/wavelength_event_broker.h"
#include "../util/wavelength_config.h"

/**
 * Klasa koordynatora sesji Wavelength - służy jako fasada dla wszystkich komponentów Wavelength,
 * upraszczając komunikację między nimi i udostępniając prosty interfejs dla WavelengthManager.
 */
class WavelengthSessionCoordinator : public QObject {
    Q_OBJECT

public:
    static WavelengthSessionCoordinator* getInstance() {
        static WavelengthSessionCoordinator instance;
        return &instance;
    }

    // Inicjalizacja koordynatora
    void initialize() {
        qDebug() << "Initializing WavelengthSessionCoordinator";

        // Połącz sygnały między komponentami
        connectSignals();

        // Załaduj konfigurację
        loadConfig();

        qDebug() << "WavelengthSessionCoordinator initialized successfully";
    }

    // ---- Metody fasadowe dla głównych operacji na wavelength ----

    // Tworzenie nowego wavelength
    bool createWavelength(double frequency, const QString& name,
                         bool isPasswordProtected, const QString& password) {
        qDebug() << "Coordinator: Creating wavelength" << frequency;
        bool success = WavelengthCreator::getInstance()->createWavelength(
            frequency, name, isPasswordProtected, password);

        if (success) {
            WavelengthStateManager::getInstance()->registerJoinedWavelength(frequency);
        }

        return success;
    }

    // Dołączanie do istniejącego wavelength
    bool joinWavelength(double frequency, const QString& password = QString()) {
        qDebug() << "Coordinator: Joining wavelength" << frequency;
        bool success = WavelengthJoiner::getInstance()->joinWavelength(frequency, password);

        if (success) {
            WavelengthStateManager::getInstance()->registerJoinedWavelength(frequency);
        }

        return success;
    }

    // Opuszczanie wavelength
    void leaveWavelength() {
        qDebug() << "Coordinator: Leaving active wavelength";
        double frequency = WavelengthStateManager::getInstance()->getActiveWavelength();

        if (frequency != -1) {
            WavelengthStateManager::getInstance()->unregisterJoinedWavelength(frequency);
        }

        WavelengthLeaver::getInstance()->leaveWavelength();
    }

    // Zamykanie wavelength (tylko dla hosta)
    void closeWavelength(double frequency) {
        qDebug() << "Coordinator: Closing wavelength" << frequency;
        WavelengthStateManager::getInstance()->unregisterJoinedWavelength(frequency);
        WavelengthLeaver::getInstance()->closeWavelength(frequency);
    }

    // Wysyłanie wiadomości
    bool sendMessage(const QString& message) {
        qDebug() << "Coordinator: Sending message to active wavelength";
        return WavelengthMessageService::getInstance()->sendMessage(message);
    }

    // ---- Metody zarządzania stanem ----

    // Pobieranie informacji o wavelength
    WavelengthInfo getWavelengthInfo(double frequency, bool* isHost = nullptr) {
        return WavelengthStateManager::getInstance()->getWavelengthInfo(frequency, isHost);
    }

    // Aktywny wavelength
    double getActiveWavelength() {
        return WavelengthStateManager::getInstance()->getActiveWavelength();
    }

    void setActiveWavelength(double frequency) {
        WavelengthStateManager::getInstance()->setActiveWavelength(frequency);
    }

    // Sprawdzanie czy użytkownik jest hostem aktywnego wavelength
    bool isActiveWavelengthHost() {
        return WavelengthStateManager::getInstance()->isActiveWavelengthHost();
    }

    // Lista dołączonych wavelength
    QList<double> getJoinedWavelengths() {
        return WavelengthStateManager::getInstance()->getJoinedWavelengths();
    }

    // Liczba dołączonych wavelength
    int getJoinedWavelengthCount() {
        return WavelengthStateManager::getInstance()->getJoinedWavelengthCount();
    }

    // Sprawdzanie czy wavelength jest chroniony hasłem
    bool isWavelengthPasswordProtected(double frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthPasswordProtected(frequency);
    }

    // Sprawdzanie czy jesteśmy hostem dla danego wavelength
    bool isWavelengthHost(double frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthHost(frequency);
    }

    // Nazwa wavelength
    QString getWavelengthName(double frequency) {
        return WavelengthStateManager::getInstance()->getWavelengthName(frequency);
    }

    // Sprawdzanie czy dołączyliśmy do wavelength
    bool isWavelengthJoined(double frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthJoined(frequency);
    }

    // Sprawdzanie czy wavelength jest podłączony
    bool isWavelengthConnected(double frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthConnected(frequency);
    }

    // ---- Metody konfiguracji ----

    // Adres serwera relay
    QString getRelayServerAddress() const {
        return WavelengthConfig::getInstance()->getRelayServerAddress();
    }

    void setRelayServerAddress(const QString& address) {
        WavelengthConfig::getInstance()->setRelayServerAddress(address);
    }

    // Pełny URL serwera relay
    QString getRelayServerUrl() const {
        return WavelengthConfig::getInstance()->getRelayServerUrl();
    }

signals:
    // Przekazywane sygnały dla WavelengthManager
    void wavelengthCreated(double frequency);
    void wavelengthJoined(double frequency);
    void wavelengthLeft(double frequency);
    void wavelengthClosed(double frequency);
    void messageReceived(double frequency, const QString& message);
    void messageSent(double frequency, const QString& message);
    void connectionError(const QString& errorMessage);
    void authenticationFailed(double frequency);
    void userKicked(double frequency, const QString& reason);
    void activeWavelengthChanged(double frequency);

private slots:
    // Obsługa zdarzeń z różnych komponentów - tutaj naprawiamy propagację sygnałów
    void onWavelengthCreated(double frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthCreated signal for frequency" << frequency;
        emit wavelengthCreated(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthCreated(frequency);
    }

    void onWavelengthJoined(double frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthJoined signal for frequency" << frequency;
        emit wavelengthJoined(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthJoined(frequency);
    }

    void onWavelengthLeft(double frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthLeft signal for frequency" << frequency;
        emit wavelengthLeft(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthLeft(frequency);
    }

    void onWavelengthClosed(double frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthClosed signal for frequency" << frequency;
        emit wavelengthClosed(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthClosed(frequency);
    }

    void onMessageReceived(double frequency, const QString& message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageReceived signal";
        emit messageReceived(frequency, message);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->messageReceived(frequency, message);
    }

    void onMessageSent(double frequency, const QString& message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageSent signal";
        emit messageSent(frequency, message);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->messageSent(frequency, message);
    }

    void onConnectionError(const QString& errorMessage) {
        qDebug() << "WavelengthSessionCoordinator: Propagating connectionError signal:" << errorMessage;
        emit connectionError(errorMessage);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->connectionError(errorMessage);
    }

    void onAuthenticationFailed(double frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating authenticationFailed signal for frequency" << frequency;
        emit authenticationFailed(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->authenticationFailed(frequency);
    }

    void onActiveWavelengthChanged(double frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating activeWavelengthChanged signal for frequency" << frequency;
        emit activeWavelengthChanged(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->activeWavelengthChanged(frequency);
    }

    void onConfigChanged(const QString& key) {
        qDebug() << "Configuration changed:" << key;
    }

private:
    WavelengthSessionCoordinator(QObject* parent = nullptr) : QObject(parent) {
        // Konstruktor prywatny dla singletona
    }

    ~WavelengthSessionCoordinator() {}

    WavelengthSessionCoordinator(const WavelengthSessionCoordinator&) = delete;
    WavelengthSessionCoordinator& operator=(const WavelengthSessionCoordinator&) = delete;

    void connectSignals() {
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
        connect(WavelengthConfig::getInstance(), &WavelengthConfig::configChanged,
                this, &WavelengthSessionCoordinator::onConfigChanged, Qt::DirectConnection);
    }

    void loadConfig() {
        // Inicjalizacja konfiguracji
        WavelengthConfig* config = WavelengthConfig::getInstance();
        
        // Możemy tu ustawić domyślne wartości, jeśli plik konfiguracyjny nie istnieje
        if (!QFile::exists(config->getSetting("configFilePath").toString())) {
            qDebug() << "Setting default configuration";
            
            // Przykładowe wartości domyślne
            config->setRelayServerAddress("localhost");
            config->setRelayServerPort(3000);
            config->setMaxChatHistorySize(200);
            config->setDebugMode(false);
            
            // Zapisz konfigurację
            config->saveSettings();
        }
    }
};

#endif // WAVELENGTH_SESSION_COORDINATOR_H