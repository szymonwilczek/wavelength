#ifndef WAVELENGTH_CONFIG_H
#define WAVELENGTH_CONFIG_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QDebug>
#include <QHostAddress>
#include <QMap>
#include <QVariant>

class WavelengthConfig : public QObject {
    Q_OBJECT

public:
    static WavelengthConfig* getInstance() {
        static WavelengthConfig instance;
        return &instance;
    }

    // Adres serwera relay
    QString getRelayServerAddress() const {
        return m_settings.value("server/relay_address", "localhost").toString();
    }

    void setRelayServerAddress(const QString& address) {
        m_settings.setValue("server/relay_address", address);
        emit configChanged("relay_server_address");
    }

    // Port serwera relay
    int getRelayServerPort() const {
        return m_settings.value("server/relay_port", 3000).toInt();
    }

    void setRelayServerPort(int port) {
        m_settings.setValue("server/relay_port", port);
        emit configChanged("relay_server_port");
    }

    // Pełny URL serwera relay
    QString getRelayServerUrl() const {
        return QString("ws://%1:%2").arg(getRelayServerAddress()).arg(getRelayServerPort());
    }

    // Maksymalna liczba wiadomości przechowywanych w historii
    int getMaxChatHistorySize() const {
        return m_settings.value("chat/max_history_size", 200).toInt();
    }

    void setMaxChatHistorySize(int size) {
        m_settings.setValue("chat/max_history_size", size);
        emit configChanged("max_chat_history_size");
    }

    // Maksymalna liczba przetworzonych ID wiadomości w pamięci podręcznej
    int getMaxProcessedMessageIds() const {
        return m_settings.value("messages/max_processed_ids", 200).toInt();
    }

    void setMaxProcessedMessageIds(int size) {
        m_settings.setValue("messages/max_processed_ids", size);
        emit configChanged("max_processed_message_ids");
    }

    // Maksymalny rozmiar pamięci podręcznej wysłanych wiadomości
    int getMaxSentMessageCacheSize() const {
        return m_settings.value("messages/max_sent_cache_size", 100).toInt();
    }

    void setMaxSentMessageCacheSize(int size) {
        m_settings.setValue("messages/max_sent_cache_size", size);
        emit configChanged("max_sent_message_cache_size");
    }

    // Timeout połączenia (w milisekundach)
    int getConnectionTimeout() const {
        return m_settings.value("connection/timeout_ms", 5000).toInt();
    }

    void setConnectionTimeout(int timeoutMs) {
        m_settings.setValue("connection/timeout_ms", timeoutMs);
        emit configChanged("connection_timeout");
    }

    // Interwał keep-alive (w milisekundach)
    int getKeepAliveInterval() const {
        return m_settings.value("connection/keep_alive_ms", 30000).toInt();
    }

    void setKeepAliveInterval(int intervalMs) {
        m_settings.setValue("connection/keep_alive_ms", intervalMs);
        emit configChanged("keep_alive_interval");
    }

    // Maksymalna liczba prób ponownego połączenia
    int getMaxReconnectAttempts() const {
        return m_settings.value("connection/max_reconnect_attempts", 3).toInt();
    }

    void setMaxReconnectAttempts(int attempts) {
        m_settings.setValue("connection/max_reconnect_attempts", attempts);
        emit configChanged("max_reconnect_attempts");
    }

    // Ustawienie trybu debugowania
    bool isDebugMode() const {
        return m_settings.value("app/debug_mode", false).toBool();
    }

    void setDebugMode(bool enabled) {
        m_settings.setValue("app/debug_mode", enabled);
        emit configChanged("debug_mode");
    }

    // Nazwa użytkownika
    QString getUserName() const {
        return m_settings.value("user/name", "Anonymous").toString();
    }

    void setUserName(const QString& name) {
        m_settings.setValue("user/name", name);
        emit configChanged("user_name");
    }

    // Motyw aplikacji
    QString getApplicationTheme() const {
        return m_settings.value("app/theme", "light").toString();
    }

    void setApplicationTheme(const QString& theme) {
        m_settings.setValue("app/theme", theme);
        emit configChanged("application_theme");
    }

    // Maksymalna liczba przechowywanych dołączonych wavelength w historii
    int getMaxRecentWavelength() const {
        return m_settings.value("wavelength/max_recent", 10).toInt();
    }

    void setMaxRecentWavelength(int count) {
        m_settings.setValue("wavelength/max_recent", count);
        emit configChanged("max_recent_wavelength");
    }

    // Zachowaj ustawienia
    void saveSettings() {
        m_settings.sync();
        qDebug() << "Settings saved to:" << m_settings.fileName();
    }

    // Przywróć domyślne ustawienia
    void restoreDefaults() {
        m_settings.clear();
        emit configChanged("all");
    }

    // Metoda do odczytywania dowolnego ustawienia
    QVariant getSetting(const QString& key, const QVariant& defaultValue = QVariant()) const {
        return m_settings.value(key, defaultValue);
    }

    // Metoda do zapisywania dowolnego ustawienia
    void setSetting(const QString& key, const QVariant& value) {
        m_settings.setValue(key, value);
        emit configChanged(key);
    }

signals:
    void configChanged(const QString& key);

private:
    WavelengthConfig(QObject* parent = nullptr) : QObject(parent), 
        m_settings("Wavelength", "WavelengthApp") {
        qDebug() << "Config initialized. Settings file:" << m_settings.fileName();
    }
    
    ~WavelengthConfig() {
        saveSettings();
    }

    WavelengthConfig(const WavelengthConfig&) = delete;
    WavelengthConfig& operator=(const WavelengthConfig&) = delete;

    QSettings m_settings;
};

#endif // WAVELENGTH_CONFIG_H