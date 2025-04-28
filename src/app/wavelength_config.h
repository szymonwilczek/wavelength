#ifndef WAVELENGTH_CONFIG_H
#define WAVELENGTH_CONFIG_H


#include <QColor>
#include <QString>
#include <QSettings>
#include <QHostAddress>
#include <QMap>
#include <QVariant>
#include <QKeySequence>

class WavelengthConfig : public QObject {
    Q_OBJECT
    static constexpr int MAX_RECENT_COLORS = 5;
public:
    static WavelengthConfig* getInstance();
    ~WavelengthConfig() override = default;

    // --- Istniejące metody ---
    QString getRelayServerAddress() const;
    void setRelayServerAddress(const QString& address);
    int getRelayServerPort() const;
    void setRelayServerPort(int port);
    QString getRelayServerUrl() const;
    int getMaxChatHistorySize() const;
    void setMaxChatHistorySize(int size);
    int getMaxProcessedMessageIds() const;
    void setMaxProcessedMessageIds(int size);
    int getMaxSentMessageCacheSize() const;
    void setMaxSentMessageCacheSize(int size);
    int getMaxRecentWavelength() const;
    void setMaxRecentWavelength(int size);
    int getConnectionTimeout() const;
    void setConnectionTimeout(int timeout);
    int getKeepAliveInterval() const;
    void setKeepAliveInterval(int interval);
    int getMaxReconnectAttempts() const;
    void setMaxReconnectAttempts(int attempts);
    bool isDebugMode() const;
    void setDebugMode(bool enabled);

    QString getPreferredStartFrequency() const;
    void setPreferredStartFrequency(QString frequency);

    // --- Kolory wyglądu ---
    QColor getBackgroundColor() const;
    void setBackgroundColor(const QColor& color);
    QColor getBlobColor() const;
    void setBlobColor(const QColor& color);
    QColor getMessageColor() const;
    void setMessageColor(const QColor& color);
    QColor getStreamColor() const;
    void setStreamColor(const QColor& color);

    // --- NOWE: Ustawienia siatki ---
    QColor getGridColor() const;
    void setGridColor(const QColor& color);
    int getGridSpacing() const;
    void setGridSpacing(int spacing);
    // -----------------------------

    // --- NOWE: Ustawienia etykiety tytułowej ---
    QColor getTitleTextColor() const;
    void setTitleTextColor(const QColor& color);
    QColor getTitleBorderColor() const;
    void setTitleBorderColor(const QColor& color);
    QColor getTitleGlowColor() const;
    void setTitleGlowColor(const QColor& color);
    // -----------------------------------------

    // --- Ostatnie kolory ---
    QStringList getRecentColors() const;
    void addRecentColor(const QColor& color);
    // -----------------------

    QKeySequence getShortcut(const QString& actionId, const QKeySequence& defaultSequence = QKeySequence()) const;
    void setShortcut(const QString& actionId, const QKeySequence& sequence);
    QMap<QString, QKeySequence> getAllShortcuts() const;
    QMap<QString, QKeySequence> getDefaultShortcutsMap() const; // <<< NOWA METODA

    void saveSettings(); // Zapisuje wszystkie bieżące wartości
    void restoreDefaults(); // Przywraca wartości domyślne

    QVariant getSetting(const QString& key) const;

signals:
    void configChanged(const QString& key);
    void recentColorsChanged();

private:
    explicit WavelengthConfig(QObject *parent = nullptr);
    WavelengthConfig(const WavelengthConfig&) = delete;
    WavelengthConfig& operator=(const WavelengthConfig&) = delete;

    void loadDefaults();
    void loadSettings(); // Ładuje z QSettings

    static WavelengthConfig* m_instance;
    QSettings m_settings;

    void loadDefaultShortcuts();
    QMap<QString, QKeySequence> m_defaultShortcuts;
    QMap<QString, QKeySequence> m_shortcuts;

    // Zmienne przechowujące wartości
    QString m_relayServerAddress;
    int m_relayServerPort;
    int m_maxChatHistorySize;
    int m_maxProcessedMessageIds;
    int m_maxSentMessageCacheSize;
    int m_maxRecentWavelength;
    int m_connectionTimeout;
    int m_keepAliveInterval;
    int m_maxReconnectAttempts;
    bool m_debugMode;
    QColor m_backgroundColor;
    QColor m_blobColor;
    QColor m_streamColor;
    QStringList m_recentColors;

    // NOWE zmienne
    QColor m_gridColor;
    int m_gridSpacing;
    QColor m_titleTextColor;
    QColor m_titleBorderColor;
    QColor m_titleGlowColor;

    QString m_preferredStartFrequency;
};

#endif // WAVELENGTH_CONFIG_H