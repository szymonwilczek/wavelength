#ifndef WAVELENGTH_SESSION_COORDINATOR_H
#define WAVELENGTH_SESSION_COORDINATOR_H

#include <QObject>
#include <QString>
#include <QDebug>
#include <qfile.h>

#include "events/creator/wavelength_creator.h"
#include "events/joiner/wavelength_joiner.h"
#include "events/leaver/wavelength_leaver.h"
#include "../chat/messages/services/wavelength_message_service.h"
#include "../chat/messages/services//wavelength_message_processor.h"
#include "../services/wavelength_state_manager.h"
#include "../services/wavelength_event_broker.h"
#include "../../src/app/wavelength_config.h"

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
    void initialize();

    // ---- Metody fasadowe dla głównych operacji na wavelength ----

    // Tworzenie nowego wavelength
    bool createWavelength(QString frequency,
                         bool isPasswordProtected, const QString& password);

    // Dołączanie do istniejącego wavelength
    bool joinWavelength(QString frequency, const QString& password = QString());

    // Opuszczanie wavelength
    void leaveWavelength();

    // Zamykanie wavelength (tylko dla hosta)
    static void closeWavelength(QString frequency);

    // Wysyłanie wiadomości
    bool sendMessage(const QString& message);

    // ---- Metody zarządzania stanem ----

    // Pobieranie informacji o wavelength
    WavelengthInfo getWavelengthInfo(QString frequency, bool* isHost = nullptr) {
        return WavelengthStateManager::getInstance()->getWavelengthInfo(frequency, isHost);
    }

    // Aktywny wavelength
    QString getActiveWavelength() {
        return WavelengthStateManager::getInstance()->getActiveWavelength();
    }

    void setActiveWavelength(QString frequency) {
        WavelengthStateManager::getInstance()->setActiveWavelength(frequency);
    }

    // Sprawdzanie czy użytkownik jest hostem aktywnego wavelength
    bool isActiveWavelengthHost() {
        return WavelengthStateManager::getInstance()->isActiveWavelengthHost();
    }

    // Lista dołączonych wavelength
    QList<QString> getJoinedWavelengths() {
        return WavelengthStateManager::getInstance()->getJoinedWavelengths();
    }

    // Liczba dołączonych wavelength
    int getJoinedWavelengthCount() {
        return WavelengthStateManager::getInstance()->getJoinedWavelengthCount();
    }

    // Sprawdzanie czy wavelength jest chroniony hasłem
    bool isWavelengthPasswordProtected(QString frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthPasswordProtected(frequency);
    }

    // Sprawdzanie czy jesteśmy hostem dla danego wavelength
    bool isWavelengthHost(QString frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthHost(frequency);
    }


    // Sprawdzanie czy dołączyliśmy do wavelength
    bool isWavelengthJoined(QString frequency) {
        return WavelengthStateManager::getInstance()->isWavelengthJoined(frequency);
    }

    // Sprawdzanie czy wavelength jest podłączony
    bool isWavelengthConnected(QString frequency) {
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
    void wavelengthCreated(QString frequency);
    void wavelengthJoined(QString frequency);
    void wavelengthLeft(QString frequency);
    void wavelengthClosed(QString frequency);
    void messageReceived(QString frequency, const QString& message);
    void messageSent(QString frequency, const QString& message);
    void connectionError(const QString& errorMessage);
    void authenticationFailed(QString frequency);
    void userKicked(QString frequency, const QString& reason);
    void activeWavelengthChanged(QString frequency);
    void pttGranted(QString frequency);
    void pttDenied(QString frequency, QString reason);
    void pttStartReceiving(QString frequency, QString senderId);
    void pttStopReceiving(QString frequency);
    void audioDataReceived(QString frequency, const QByteArray& audioData);
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private slots:
    // Obsługa zdarzeń z różnych komponentów - tutaj naprawiamy propagację sygnałów
    void onWavelengthCreated(QString frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthCreated signal for frequency" << frequency;
        emit wavelengthCreated(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthCreated(frequency);
    }

    void onWavelengthJoined(QString frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthJoined signal for frequency" << frequency;
        emit wavelengthJoined(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthJoined(frequency);
    }

    void onWavelengthLeft(QString frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthLeft signal for frequency" << frequency;
        emit wavelengthLeft(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthLeft(frequency);
    }

    void onWavelengthClosed(QString frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthClosed signal for frequency" << frequency;
        emit wavelengthClosed(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->wavelengthClosed(frequency);
    }

    void onMessageReceived(QString frequency, const QString& message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageReceived signal";
        emit messageReceived(frequency, message);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->messageReceived(frequency, message);
    }

    void onMessageSent(QString frequency, const QString& message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageSent signal";
        emit messageSent(frequency, message);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->messageSent(frequency, message);
    }

    void onConnectionError(const QString& errorMessage) {
        qDebug() << "WavelengthSessionCoordinator: Propagating connectionError signal:" << errorMessage;
        emit connectionError(errorMessage);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->connectionError(errorMessage);
    }

    void onAuthenticationFailed(QString frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating authenticationFailed signal for frequency" << frequency;
        emit authenticationFailed(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->authenticationFailed(frequency);
    }

    void onActiveWavelengthChanged(QString frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating activeWavelengthChanged signal for frequency" << frequency;
        emit activeWavelengthChanged(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::getInstance()->activeWavelengthChanged(frequency);
    }

    void onConfigChanged(const QString& key) {
        qDebug() << "Configuration changed:" << key;
    }

    // --- NOWE SLOTY PTT (odbierające z WavelengthMessageService) ---
    void onPttGranted(QString frequency) { emit pttGranted(frequency); }
    void onPttDenied(QString frequency, QString reason) { emit pttDenied(frequency, reason); }
    void onPttStartReceiving(QString frequency, QString senderId) { emit pttStartReceiving(frequency, senderId); }
    void onPttStopReceiving(QString frequency) { emit pttStopReceiving(frequency); }
    void onAudioDataReceived(QString frequency, const QByteArray& audioData) { emit audioDataReceived(frequency, audioData); }
    void onRemoteAudioAmplitudeUpdate(QString frequency, qreal amplitude) { emit remoteAudioAmplitudeUpdate(frequency, amplitude); }
    // --- KONIEC NOWYCH SLOTÓW PTT ---

private:
    WavelengthSessionCoordinator(QObject* parent = nullptr) : QObject(parent) {
        // Konstruktor prywatny dla singletona
    }

    ~WavelengthSessionCoordinator() {}

    WavelengthSessionCoordinator(const WavelengthSessionCoordinator&) = delete;
    WavelengthSessionCoordinator& operator=(const WavelengthSessionCoordinator&) = delete;

    void connectSignals();

    static void loadConfig();
};

#endif // WAVELENGTH_SESSION_COORDINATOR_H