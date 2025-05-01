#ifndef WAVELENGTH_SESSION_COORDINATOR_H
#define WAVELENGTH_SESSION_COORDINATOR_H

#include <QObject>
#include <QString>

#include "events/joiner/wavelength_joiner.h"
#include "../chat/messages/services/wavelength_message_service.h"
#include "../services/wavelength_state_manager.h"
#include "../services/wavelength_event_broker.h"
#include "../../src/app/wavelength_config.h"

/**
 * Klasa koordynatora sesji Wavelength - służy jako fasada dla wszystkich komponentów Wavelength,
 * upraszczając komunikację między nimi i udostępniając prosty interfejs dla WavelengthManager.
 */
class WavelengthSessionCoordinator final : public QObject {
    Q_OBJECT

public:
    static WavelengthSessionCoordinator* GetInstance() {
        static WavelengthSessionCoordinator instance;
        return &instance;
    }

    // Inicjalizacja koordynatora
    void Initialize();

    // ---- Metody fasadowe dla głównych operacji na wavelength ----

    // Tworzenie nowego wavelength
    static bool CreateWavelength(const QString &frequency,
                                 bool is_password_protected, const QString& password);

    // Dołączanie do istniejącego wavelength
    static bool JoinWavelength(const QString &frequency, const QString& password = QString());

    // Opuszczanie wavelength
    static void LeaveWavelength();

    // Zamykanie wavelength (tylko dla hosta)
    static void CloseWavelength(const QString &frequency);

    // Wysyłanie wiadomości
    static bool SendMessage(const QString& message);

    // ---- Metody zarządzania stanem ----

    // Pobieranie informacji o wavelength
    static WavelengthInfo GetWavelengthInfo(const QString &frequency, bool* is_host = nullptr) {
        return WavelengthStateManager::GetInstance()->GetWavelengthInfo(frequency, is_host);
    }

    // Aktywny wavelength
    static QString GetActiveWavelength() {
        return WavelengthStateManager::GetInstance()->GetActiveWavelength();
    }

    static void SetActiveWavelength(const QString &frequency) {
        WavelengthStateManager::GetInstance()->SetActiveWavelength(frequency);
    }

    // Sprawdzanie czy użytkownik jest hostem aktywnego wavelength
    static bool IsActiveWavelengthHost() {
        return WavelengthStateManager::GetInstance()->IsActiveWavelengthHost();
    }

    // Lista dołączonych wavelength
    static QList<QString> GetJoinedWavelengths() {
        return WavelengthStateManager::GetInstance()->GetJoinedWavelengths();
    }

    // Liczba dołączonych wavelength
    static int GetJoinedWavelengthCount() {
        return WavelengthStateManager::GetInstance()->GetJoinedWavelengthCount();
    }

    // Sprawdzanie czy wavelength jest chroniony hasłem
    static bool IsWavelengthPasswordProtected(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthPasswordProtected(frequency);
    }

    // Sprawdzanie czy jesteśmy hostem dla danego wavelength
    static bool IsWavelengthHost(const QString &frequency) {
        return WavelengthStateManager::GetInstance()->IsWavelengthHost(frequency);
    }


    // Sprawdzanie czy dołączyliśmy do wavelength
    static bool IsWavelengthJoined(const QString &frequency);

    // Sprawdzanie czy wavelength jest podłączony
    static bool IsWavelengthConnected(const QString &frequency);

    // ---- Metody konfiguracji ----

    // Adres serwera relay
    static QString GetRelayServerAddress() {
        return WavelengthConfig::GetInstance()->GetRelayServerAddress();
    }

    static void SetRelayServerAddress(const QString& address) {
        WavelengthConfig::GetInstance()->SetRelayServerAddress(address);
    }

    // Pełny URL serwera relay
    static QString GetRelayServerUrl() {
        return WavelengthConfig::GetInstance()->GetRelayServerUrl();
    }

signals:
    void wavelengthCreated(QString frequency);
    void wavelengthJoined(QString frequency);
    void wavelengthLeft(QString frequency);
    void wavelengthClosed(QString frequency);
    void messageReceived(QString frequency, const QString& message);
    void messageSent(QString frequency, const QString& message);
    void connectionError(const QString& error_message);
    void authenticationFailed(QString frequency);
    void userKicked(QString frequency, const QString& reason);
    void activeWavelengthChanged(QString frequency);
    void pttGranted(QString frequency);
    void pttDenied(QString frequency, QString reason);
    void pttStartReceiving(QString frequency, QString sender_id);
    void pttStopReceiving(QString frequency);
    void audioDataReceived(QString frequency, const QByteArray& audio_data);
    void remoteAudioAmplitudeUpdate(QString frequency, qreal amplitude);

private slots:
    void onWavelengthCreated(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthCreated signal for frequency" << frequency;
        emit wavelengthCreated(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->WavelengthCreated(frequency);
    }

    void onWavelengthJoined(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthJoined signal for frequency" << frequency;
        emit wavelengthJoined(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->WavelengthJoined(frequency);
    }

    void onWavelengthLeft(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthLeft signal for frequency" << frequency;
        emit wavelengthLeft(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->WavelengthLeft(frequency);
    }

    void onWavelengthClosed(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating wavelengthClosed signal for frequency" << frequency;
        emit wavelengthClosed(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->WavelengthClosed(frequency);
    }

    void onMessageReceived(const QString &frequency, const QString& message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageReceived signal";
        emit messageReceived(frequency, message);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->MessageReceived(frequency, message);
    }

    void onMessageSent(const QString &frequency, const QString& message) {
        qDebug() << "WavelengthSessionCoordinator: Propagating messageSent signal";
        emit messageSent(frequency, message);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->MessageSent(frequency, message);
    }

    void onConnectionError(const QString& errorMessage) {
        qDebug() << "WavelengthSessionCoordinator: Propagating connectionError signal:" << errorMessage;
        emit connectionError(errorMessage);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->ConnectionError(errorMessage);
    }

    void onAuthenticationFailed(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating authenticationFailed signal for frequency" << frequency;
        emit authenticationFailed(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->AuthenticationFailed(frequency);
    }

    void onActiveWavelengthChanged(const QString &frequency) {
        qDebug() << "WavelengthSessionCoordinator: Propagating activeWavelengthChanged signal for frequency" << frequency;
        emit activeWavelengthChanged(frequency);  // BEZPOŚREDNIE EMITOWANIE SYGNAŁU
        WavelengthEventBroker::GetInstance()->ActiveWavelengthChanged(frequency);
    }

    static void onConfigChanged(const QString& key) {
        qDebug() << "Configuration changed:" << key;
    }

    // --- NOWE SLOTY PTT (odbierające z WavelengthMessageService) ---
    void onPttGranted(const QString &frequency) { emit pttGranted(frequency); }
    void onPttDenied(const QString &frequency, const QString &reason) { emit pttDenied(frequency, reason); }
    void onPttStartReceiving(const QString &frequency, const QString &sender_id) { emit pttStartReceiving(frequency, sender_id); }
    void onPttStopReceiving(const QString &frequency) { emit pttStopReceiving(frequency); }
    void onAudioDataReceived(const QString &frequency, const QByteArray& audio_data) { emit audioDataReceived(frequency, audio_data); }
    void onRemoteAudioAmplitudeUpdate(const QString &frequency, const qreal amplitude) { emit remoteAudioAmplitudeUpdate(frequency, amplitude); }
    // --- KONIEC NOWYCH SLOTÓW PTT ---

private:
    explicit WavelengthSessionCoordinator(QObject* parent = nullptr) : QObject(parent) {
        // Konstruktor prywatny dla singletona
    }

    ~WavelengthSessionCoordinator() override {}

    WavelengthSessionCoordinator(const WavelengthSessionCoordinator&) = delete;
    WavelengthSessionCoordinator& operator=(const WavelengthSessionCoordinator&) = delete;

    void ConnectSignals();

    static void LoadConfig();
};

#endif // WAVELENGTH_SESSION_COORDINATOR_H