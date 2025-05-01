#ifndef WAVELENGTH_CONFIG_H
#define WAVELENGTH_CONFIG_H

#include <QColor>
#include <QString>
#include <QSettings>
#include <QHostAddress>
#include <QMap>
#include <QVariant>
#include <QKeySequence>

class WavelengthConfig final : public QObject {
    Q_OBJECT
    static constexpr int kMaxRecentColors = 5;
    const QString kShortcutsPrefix = "Shortcuts/";
public:
    static WavelengthConfig* GetInstance();
    ~WavelengthConfig() override = default;

    QString GetRelayServerAddress() const;
    void SetRelayServerAddress(const QString& address);
    int GetRelayServerPort() const;
    void SetRelayServerPort(int port);
    QString GetRelayServerUrl() const;
    int GetMaxChatHistorySize() const;
    void SetMaxChatHistorySize(int size);
    int GetMaxProcessedMessageIds() const;
    void SetMaxProcessedMessageIds(int size);
    int GetMaxSentMessageCacheSize() const;
    void SetMaxSentMessageCacheSize(int size);
    int GetMaxRecentWavelength() const;
    void SetMaxRecentWavelength(int size);
    int GetConnectionTimeout() const;
    void SetConnectionTimeout(int timeout);
    int GetKeepAliveInterval() const;
    void SetKeepAliveInterval(int interval);
    int GetMaxReconnectAttempts() const;
    void SetMaxReconnectAttempts(int attempts);
    bool IsDebugMode() const;
    void SetDebugMode(bool enabled);

    QString GetPreferredStartFrequency() const;
    void SetPreferredStartFrequency(const QString &frequency);

    // --- Kolory wyglądu ---
    QColor GetBackgroundColor() const;
    void SetBackgroundColor(const QColor& color);
    QColor GetBlobColor() const;
    void SetBlobColor(const QColor& color);
    QColor GetStreamColor() const;
    void SetStreamColor(const QColor& color);

    // --- NOWE: Ustawienia siatki ---
    QColor GetGridColor() const;
    void SetGridColor(const QColor& color);
    int GetGridSpacing() const;
    void SetGridSpacing(int spacing);
    // -----------------------------

    // --- NOWE: Ustawienia etykiety tytułowej ---
    QColor GetTitleTextColor() const;
    void SetTitleTextColor(const QColor& color);
    QColor GetTitleBorderColor() const;
    void SetTitleBorderColor(const QColor& color);
    QColor GetTitleGlowColor() const;
    void SetTitleGlowColor(const QColor& color);
    // -----------------------------------------

    // --- Ostatnie kolory ---
    QStringList GetRecentColors() const;
    void AddRecentColor(const QColor& color);
    // -----------------------

    QKeySequence GetShortcut(const QString& action_id, const QKeySequence& default_sequence = QKeySequence()) const;
    void SetShortcut(const QString& action_id, const QKeySequence& sequence);
    QMap<QString, QKeySequence> GetAllShortcuts() const;
    QMap<QString, QKeySequence> GetDefaultShortcutsMap() const; // <<< NOWA METODA

    void SaveSettings(); // Zapisuje wszystkie bieżące wartości
    void RestoreDefaults(); // Przywraca wartości domyślne

    QVariant GetSetting(const QString& key) const;

signals:
    void configChanged(const QString& key);
    void recentColorsChanged();

private:
    explicit WavelengthConfig(QObject *parent = nullptr);
    WavelengthConfig(const WavelengthConfig&) = delete;
    WavelengthConfig& operator=(const WavelengthConfig&) = delete;

    void LoadDefaults();
    void LoadSettings(); // Ładuje z QSettings

    static WavelengthConfig* instance_;
    QSettings settings_;

    void LoadDefaultShortcuts();
    QMap<QString, QKeySequence> default_shortcuts_;
    QMap<QString, QKeySequence> shortcuts_;

    // Zmienne przechowujące wartości
    QString relay_server_address_;
    int relay_server_port_;
    int max_chat_history_size_;
    int max_processed_message_ids_;
    int max_sent_message_cache_size_;
    int max_recent_wavelength_;
    int connection_timeout_;
    int keep_alive_interval_;
    int max_reconnect_attempts_;
    bool debug_mode_;
    QColor background_color_;
    QColor blob_color_;
    QColor stream_color_;
    QStringList recent_colors_;

    // NOWE zmienne
    QColor grid_color_;
    int grid_spacing_;
    QColor title_text_color_;
    QColor title_border_color_;
    QColor title_glow_color_;

    QString preferred_start_frequency_;
};

#endif // WAVELENGTH_CONFIG_H