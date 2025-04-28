#include "wavelength_config.h"

#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <qkeysequence.h>

const QString SHORTCUTS_PREFIX = "Shortcuts/";

namespace DefaultConfig {
    const QString RELAY_SERVER_ADDRESS = "127.0.0.1";
    constexpr int RELAY_SERVER_PORT = 8080;
    constexpr int MAX_CHAT_HISTORY_SIZE = 1000;
    constexpr int MAX_PROCESSED_MESSAGE_IDS = 5000;
    constexpr int MAX_SENT_MESSAGE_CACHE_SIZE = 100;
    constexpr int MAX_RECENT_WAVELENGTH = 10;
    constexpr int CONNECTION_TIMEOUT = 5000; // ms
    constexpr int KEEP_ALIVE_INTERVAL = 30000; // ms
    constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    constexpr bool DEBUG_MODE = false;
    const auto BACKGROUND_COLOR = QColor(0x101820); // Ciemny niebiesko-szary
    const auto BLOB_COLOR = QColor(0x4682B4); // Steel Blue
    const auto MESSAGE_COLOR = QColor(0xE0E0E0); // Jasnoszary
    const auto STREAM_COLOR = QColor(0x00FF00); // Zielony (placeholder)
    // NOWE domyślne
    constexpr auto GRID_COLOR = QColor(40, 60, 80, 150); // Półprzezroczysty ciemnoniebieski
    constexpr int GRID_SPACING = 35;
    const auto TITLE_TEXT_COLOR = QColor(0xFFFFFF); // Biały
    const auto TITLE_BORDER_COLOR = QColor(0xE0B0FF); // Lawendowy fiolet
    const auto TITLE_GLOW_COLOR = QColor(0xE0B0FF); // Lawendowy fiolet
}

WavelengthConfig* WavelengthConfig::m_instance = nullptr;

WavelengthConfig* WavelengthConfig::getInstance() {
    if (!m_instance) {
        m_instance = new WavelengthConfig();
    }
    return m_instance;
}

WavelengthConfig::WavelengthConfig(QObject *parent)
    : QObject(parent),
    m_settings(QSettings::UserScope,
     QCoreApplication::organizationName(), QCoreApplication::applicationName())
{
    qDebug() << "[Config] Initializing... Settings file:" << m_settings.fileName();
    // 1. Załaduj definicje domyślnych skrótów
    loadDefaultShortcuts();
    // 2. Ustaw domyślne wartości dla wszystkich ustawień (w tym skopiuj domyślne skróty do m_shortcuts)
    loadDefaults();
    // 3. Załaduj zapisane ustawienia, nadpisując domyślne (w tym w m_shortcuts)
    loadSettings();
    qDebug() << "[Config] Initialization complete.";
}

void WavelengthConfig::loadDefaults() {
    m_relayServerAddress = DefaultConfig::RELAY_SERVER_ADDRESS;
    m_relayServerPort = DefaultConfig::RELAY_SERVER_PORT;
    m_maxChatHistorySize = DefaultConfig::MAX_CHAT_HISTORY_SIZE;
    m_maxProcessedMessageIds = DefaultConfig::MAX_PROCESSED_MESSAGE_IDS;
    m_maxSentMessageCacheSize = DefaultConfig::MAX_SENT_MESSAGE_CACHE_SIZE;
    m_maxRecentWavelength = DefaultConfig::MAX_RECENT_WAVELENGTH;
    m_connectionTimeout = DefaultConfig::CONNECTION_TIMEOUT;
    m_keepAliveInterval = DefaultConfig::KEEP_ALIVE_INTERVAL;
    m_maxReconnectAttempts = DefaultConfig::MAX_RECONNECT_ATTEMPTS;
    m_debugMode = DefaultConfig::DEBUG_MODE;
    m_backgroundColor = DefaultConfig::BACKGROUND_COLOR;
    m_blobColor = DefaultConfig::BLOB_COLOR;
    m_streamColor = DefaultConfig::STREAM_COLOR;
    m_recentColors.clear(); // Domyślnie brak ostatnich kolorów

    // NOWE domyślne
    m_gridColor = DefaultConfig::GRID_COLOR;
    m_gridSpacing = DefaultConfig::GRID_SPACING;
    m_titleTextColor = DefaultConfig::TITLE_TEXT_COLOR;
    m_titleBorderColor = DefaultConfig::TITLE_BORDER_COLOR;
    m_titleGlowColor = DefaultConfig::TITLE_GLOW_COLOR;
    m_preferredStartFrequency = "130.0";
    m_shortcuts = m_defaultShortcuts;
}

void WavelengthConfig::loadDefaultShortcuts() {
    m_defaultShortcuts.clear();
    // Okno główne
    m_defaultShortcuts["MainWindow.CreateWavelength"] = QKeySequence("Ctrl+N");
    m_defaultShortcuts["MainWindow.JoinWavelength"] = QKeySequence("Ctrl+J");
    m_defaultShortcuts["MainWindow.OpenSettings"] = QKeySequence("Ctrl+,");
    // Widok czatu
    m_defaultShortcuts["ChatView.AbortConnection"] = QKeySequence("Ctrl+Q");
    m_defaultShortcuts["ChatView.FocusInput"] = QKeySequence("Ctrl+L");
    m_defaultShortcuts["ChatView.AttachFile"] = QKeySequence("Ctrl+O");
    m_defaultShortcuts["ChatView.SendMessage"] = QKeySequence("Ctrl+Enter");
    m_defaultShortcuts["ChatView.TogglePTT"] = QKeySequence("Space"); // Użyjemy spacji
    // Widok ustawień
    m_defaultShortcuts["SettingsView.SwitchTab0"] = QKeySequence("Ctrl+1");
    m_defaultShortcuts["SettingsView.SwitchTab1"] = QKeySequence("Ctrl+2");
    m_defaultShortcuts["SettingsView.SwitchTab2"] = QKeySequence("Ctrl+3");
    m_defaultShortcuts["SettingsView.SwitchTab3"] = QKeySequence("Ctrl+4");
    m_defaultShortcuts["SettingsView.SwitchTab4"] = QKeySequence("Ctrl+5"); // Dla nowej zakładki skrótów
    m_defaultShortcuts["SettingsView.Save"] = QKeySequence("Ctrl+S");
    m_defaultShortcuts["SettingsView.Defaults"] = QKeySequence("Ctrl+D");
    m_defaultShortcuts["SettingsView.Back"] = QKeySequence(Qt::Key_Escape);
}

void WavelengthConfig::loadSettings() {
    m_settings.beginGroup("Wavelength");
    m_preferredStartFrequency = m_settings.value("preferredStartFrequency", m_preferredStartFrequency).toDouble();
    m_settings.endGroup();

    m_settings.beginGroup("Application");
    m_maxChatHistorySize = m_settings.value("maxChatHistorySize", m_maxChatHistorySize).toInt();
    m_maxProcessedMessageIds = m_settings.value("maxProcessedMessageIds", m_maxProcessedMessageIds).toInt();
    m_maxSentMessageCacheSize = m_settings.value("maxSentMessageCacheSize", m_maxSentMessageCacheSize).toInt();
    m_maxRecentWavelength = m_settings.value("maxRecentWavelength", m_maxRecentWavelength).toInt();
    m_debugMode = m_settings.value("debugMode", m_debugMode).toBool();
    m_settings.endGroup();

    m_settings.beginGroup("Appearance");
    m_backgroundColor = m_settings.value("backgroundColor", m_backgroundColor).value<QColor>();
    m_blobColor = m_settings.value("blobColor", m_blobColor).value<QColor>();
    m_streamColor = m_settings.value("streamColor", m_streamColor).value<QColor>();
    m_recentColors = m_settings.value("recentColors", QStringList()).toStringList();
    m_gridColor = m_settings.value("gridColor", m_gridColor).value<QColor>();
    m_gridSpacing = m_settings.value("gridSpacing", m_gridSpacing).toInt();
    m_titleTextColor = m_settings.value("titleTextColor", m_titleTextColor).value<QColor>();
    m_titleBorderColor = m_settings.value("titleBorderColor", m_titleBorderColor).value<QColor>();
    m_titleGlowColor = m_settings.value("titleGlowColor", m_titleGlowColor).value<QColor>();
    m_settings.endGroup();

    m_settings.beginGroup("Network");
    m_relayServerAddress = m_settings.value("relayServerAddress", m_relayServerAddress).toString();
    m_relayServerPort = m_settings.value("relayServerPort", m_relayServerPort).toInt();
    m_connectionTimeout = m_settings.value("connectionTimeout", m_connectionTimeout).toInt();
    m_keepAliveInterval = m_settings.value("keepAliveInterval", m_keepAliveInterval).toInt();
    m_maxReconnectAttempts = m_settings.value("maxReconnectAttempts", m_maxReconnectAttempts).toInt();
    m_settings.endGroup();

    m_settings.beginGroup(SHORTCUTS_PREFIX); // Użyj grupy dla skrótów
    QStringList shortcutKeys = m_settings.childKeys();
    for (const QString& key : shortcutKeys) {
        QKeySequence sequence = m_settings.value(key).value<QKeySequence>();
        if (!sequence.isEmpty()) {
            QString actionId = key;
            m_shortcuts[actionId] = sequence;
            qDebug() << "[Config] Loaded shortcut from settings:" << actionId << "=" << sequence.toString();
        } else {
            qDebug() << "[Config] Warning: Empty shortcut found in settings for key:" << key;
            if (m_defaultShortcuts.contains(key) && !m_shortcuts.contains(key)) {
                m_shortcuts[key] = m_defaultShortcuts.value(key);
            }
        }
    }
    m_settings.endGroup(); // Koniec grupy Shortcuts
}

void WavelengthConfig::saveSettings() {
    m_settings.beginGroup("Wavelength");
    m_settings.setValue("preferredStartFrequency", m_preferredStartFrequency);
    m_settings.endGroup();

    m_settings.beginGroup("Application");
    m_settings.setValue("maxChatHistorySize", m_maxChatHistorySize);
    m_settings.setValue("maxProcessedMessageIds", m_maxProcessedMessageIds);
    m_settings.setValue("maxSentMessageCacheSize", m_maxSentMessageCacheSize);
    m_settings.setValue("maxRecentWavelength", m_maxRecentWavelength);
    m_settings.setValue("debugMode", m_debugMode);
    m_settings.endGroup();

    m_settings.beginGroup("Appearance");
    m_settings.setValue("backgroundColor", m_backgroundColor);
    m_settings.setValue("blobColor", m_blobColor);
    m_settings.setValue("streamColor", m_streamColor);
    m_settings.setValue("recentColors", m_recentColors);
    m_settings.setValue("gridColor", m_gridColor);
    m_settings.setValue("gridSpacing", m_gridSpacing);
    m_settings.setValue("titleTextColor", m_titleTextColor);
    m_settings.setValue("titleBorderColor", m_titleBorderColor);
    m_settings.setValue("titleGlowColor", m_titleGlowColor);
    m_settings.endGroup();

    m_settings.beginGroup("Network");
    m_settings.setValue("relayServerAddress", m_relayServerAddress);
    m_settings.setValue("relayServerPort", m_relayServerPort);
    m_settings.setValue("connectionTimeout", m_connectionTimeout);
    m_settings.setValue("keepAliveInterval", m_keepAliveInterval);
    m_settings.setValue("maxReconnectAttempts", m_maxReconnectAttempts);
    m_settings.endGroup();

    // --- NOWE: Zapisywanie skrótów ---
    m_settings.beginGroup(SHORTCUTS_PREFIX); // Użyj grupy dla skrótów
    // Wyczyść stare klucze, aby usunąć te, które mogły zostać przywrócone do domyślnych
    // (alternatywnie, można by zapisywać tylko te, które różnią się od domyślnych)
    m_settings.remove(""); // Usuwa wszystkie klucze w bieżącej grupie

    for (auto it = m_shortcuts.constBegin(); it != m_shortcuts.constEnd(); ++it) {
        QString actionId = it.key();
        QKeySequence sequence = it.value();
        // Opcjonalnie: Zapisuj tylko jeśli różni się od domyślnego
        // if (sequence != m_defaultShortcuts.value(actionId)) {
        m_settings.setValue(actionId, sequence); // Zapisz pod ID akcji
        qDebug() << "[Config] Saving shortcut to settings:" << actionId << "=" << sequence.toString();
        // }
    }
    m_settings.endGroup(); // Koniec grupy Shortcuts
    // --- KONIEC NOWE ---

    m_settings.sync(); // Wymuś zapis na dysk
    qDebug() << "Settings saved to:" << m_settings.fileName();
}

void WavelengthConfig::restoreDefaults() {
    loadDefaults();
    m_settings.beginGroup(SHORTCUTS_PREFIX);
    m_settings.remove(""); // Usuń wszystkie klucze w grupie skrótów
    m_settings.endGroup();
    saveSettings(); // Zapisz domyślne wartości
    emit configChanged("all"); // Sygnalizuj zmianę wszystkich ustawień
    emit recentColorsChanged(); // Zaktualizuj też ostatnie kolory (wyczyszczone)
    qDebug() << "Restored default settings.";
}

QString WavelengthConfig::getRelayServerAddress() const { return m_relayServerAddress; }
int WavelengthConfig::getRelayServerPort() const { return m_relayServerPort; }
QColor WavelengthConfig::getBackgroundColor() const { return m_backgroundColor; }
QColor WavelengthConfig::getBlobColor() const { return m_blobColor; }
QColor WavelengthConfig::getStreamColor() const { return m_streamColor; }
QStringList WavelengthConfig::getRecentColors() const { return m_recentColors; }
QColor WavelengthConfig::getGridColor() const { return m_gridColor; }
int WavelengthConfig::getGridSpacing() const { return m_gridSpacing; }
QColor WavelengthConfig::getTitleTextColor() const { return m_titleTextColor; }
QColor WavelengthConfig::getTitleBorderColor() const { return m_titleBorderColor; }
QColor WavelengthConfig::getTitleGlowColor() const { return m_titleGlowColor; }

void WavelengthConfig::setRelayServerAddress(const QString& address) {
    if (m_relayServerAddress != address) {
        m_relayServerAddress = address;
        emit configChanged("relayServerAddress");
    }
}
void WavelengthConfig::setRelayServerPort(int port) {
    if (m_relayServerPort != port) {
        m_relayServerPort = port;
        emit configChanged("relayServerPort");
    }
}
void WavelengthConfig::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color && color.isValid()) {
        m_backgroundColor = color;
        addRecentColor(color); // Dodaj do ostatnich
        emit configChanged("background_color");
        qDebug() << "Configuration changed: \"background_color\"";
    }
}
void WavelengthConfig::setBlobColor(const QColor& color) {
    if (m_blobColor != color && color.isValid()) {
        m_blobColor = color;
        addRecentColor(color);
        emit configChanged("blob_color");
        qDebug() << "Configuration changed: \"blob_color\"";
    }
}

void WavelengthConfig::setStreamColor(const QColor& color) {
    if (m_streamColor != color && color.isValid()) {
        m_streamColor = color;
        addRecentColor(color);
        emit configChanged("stream_color");
        qDebug() << "Configuration changed: \"stream_color\"";
    }
}

// --- NOWE settery ---
void WavelengthConfig::setGridColor(const QColor& color) {
    if (m_gridColor != color && color.isValid()) {
        m_gridColor = color;
        addRecentColor(color); // Kolor siatki też może być ostatnim
        emit configChanged("grid_color");
        qDebug() << "Configuration changed: \"grid_color\"";
    }
}

void WavelengthConfig::setGridSpacing(int spacing) {
    if (m_gridSpacing != spacing && spacing > 0) { // Dodaj walidację (np. > 0)
        m_gridSpacing = spacing;
        emit configChanged("grid_spacing");
        qDebug() << "Configuration changed: \"grid_spacing\"";
    }
}

void WavelengthConfig::setTitleTextColor(const QColor& color) {
    if (m_titleTextColor != color && color.isValid()) {
        m_titleTextColor = color;
        addRecentColor(color);
        emit configChanged("title_text_color");
        qDebug() << "Configuration changed: \"title_text_color\"";
    }
}

void WavelengthConfig::setTitleBorderColor(const QColor& color) {
    if (m_titleBorderColor != color && color.isValid()) {
        m_titleBorderColor = color;
        addRecentColor(color);
        emit configChanged("title_border_color");
        qDebug() << "Configuration changed: \"title_border_color\"";
    }
}

void WavelengthConfig::setTitleGlowColor(const QColor& color) {
    if (m_titleGlowColor != color && color.isValid()) {
        m_titleGlowColor = color;
        addRecentColor(color);
        emit configChanged("title_glow_color");
        qDebug() << "Configuration changed: \"title_glow_color\"";
    }
}
// --------------------

void WavelengthConfig::addRecentColor(const QColor& color) {
    if (!color.isValid()) return;
    QString hex = color.name(QColor::HexRgb); // Użyj HexRgb dla spójności
    m_recentColors.removeAll(hex); // Usuń, jeśli już istnieje
    m_recentColors.prepend(hex); // Dodaj na początek
    while (m_recentColors.size() > MAX_RECENT_COLORS) {
        m_recentColors.removeLast(); // Usuń najstarsze, jeśli przekroczono limit
    }
    emit recentColorsChanged(); // Sygnalizuj zmianę listy
}

QString WavelengthConfig::getRelayServerUrl() const {
    return QString("ws://%1:%2").arg(m_relayServerAddress).arg(m_relayServerPort);
}
int WavelengthConfig::getMaxChatHistorySize() const { return m_maxChatHistorySize; }
void WavelengthConfig::setMaxChatHistorySize(const int size) { if(m_maxChatHistorySize != size && size > 0) { m_maxChatHistorySize = size; emit configChanged("maxChatHistorySize"); } }
int WavelengthConfig::getMaxProcessedMessageIds() const { return m_maxProcessedMessageIds; }
void WavelengthConfig::setMaxProcessedMessageIds(const int size) { if(m_maxProcessedMessageIds != size && size > 0) { m_maxProcessedMessageIds = size; emit configChanged("maxProcessedMessageIds"); } }
int WavelengthConfig::getMaxSentMessageCacheSize() const { return m_maxSentMessageCacheSize; }
void WavelengthConfig::setMaxSentMessageCacheSize(const int size) { if(m_maxSentMessageCacheSize != size && size > 0) { m_maxSentMessageCacheSize = size; emit configChanged("maxSentMessageCacheSize"); } }
int WavelengthConfig::getMaxRecentWavelength() const { return m_maxRecentWavelength; }
void WavelengthConfig::setMaxRecentWavelength(const int size) { if(m_maxRecentWavelength != size && size > 0) { m_maxRecentWavelength = size; emit configChanged("maxRecentWavelength"); } }
int WavelengthConfig::getConnectionTimeout() const { return m_connectionTimeout; }
void WavelengthConfig::setConnectionTimeout(const int timeout) { if(m_connectionTimeout != timeout && timeout > 0) { m_connectionTimeout = timeout; emit configChanged("connectionTimeout"); } }
int WavelengthConfig::getKeepAliveInterval() const { return m_keepAliveInterval; }
void WavelengthConfig::setKeepAliveInterval(const int interval) { if(m_keepAliveInterval != interval && interval > 0) { m_keepAliveInterval = interval; emit configChanged("keepAliveInterval"); } }
int WavelengthConfig::getMaxReconnectAttempts() const { return m_maxReconnectAttempts; }
void WavelengthConfig::setMaxReconnectAttempts(const int attempts) { if(m_maxReconnectAttempts != attempts && attempts >= 0) { m_maxReconnectAttempts = attempts; emit configChanged("maxReconnectAttempts"); } }
bool WavelengthConfig::isDebugMode() const { return m_debugMode; }
void WavelengthConfig::setDebugMode(const bool enabled) { if(m_debugMode != enabled) { m_debugMode = enabled; emit configChanged("debugMode"); } }

QKeySequence WavelengthConfig::getShortcut(const QString& actionId, const QKeySequence& defaultSequence) const {
    // Odczytaj z mapy m_shortcuts, używając domyślnego jako fallback
    const QKeySequence actualDefault = defaultSequence.isEmpty() ? m_defaultShortcuts.value(actionId) : defaultSequence;
    QKeySequence result = m_shortcuts.value(actionId, actualDefault);
    qDebug() << "[Config] getShortcut(" << actionId << "): Returning from map:" << result.toString();
    return result;
}

void WavelengthConfig::setShortcut(const QString& actionId, const QKeySequence& sequence) {
    // Zaktualizuj mapę m_shortcuts
    if (m_shortcuts.value(actionId) != sequence) {
        qDebug() << "[Config] setShortcut(" << actionId << "): Updating map to" << sequence.toString();
        m_shortcuts[actionId] = sequence;
        // Zapis do QSettings nastąpi podczas saveSettings()
    }
}

QMap<QString, QKeySequence> WavelengthConfig::getAllShortcuts() const {
    // Zwróć kopię mapy m_shortcuts
    return m_shortcuts;
}

QMap<QString, QKeySequence> WavelengthConfig::getDefaultShortcutsMap() const {
    return m_defaultShortcuts; // Zwróć kopię mapy domyślnych
}

QVariant WavelengthConfig::getSetting(const QString& key) const {
    if (key == "relayServerAddress") return m_relayServerAddress;
    if (key == "relayServerPort") return m_relayServerPort;
    if (key == "maxChatHistorySize") return m_maxChatHistorySize;
    if (key == "maxProcessedMessageIds") return m_maxProcessedMessageIds;
    if (key == "maxSentMessageCacheSize") return m_maxSentMessageCacheSize;
    if (key == "maxRecentWavelength") return m_maxRecentWavelength;
    if (key == "connectionTimeout") return m_connectionTimeout;
    if (key == "keepAliveInterval") return m_keepAliveInterval;
    if (key == "maxReconnectAttempts") return m_maxReconnectAttempts;
    if (key == "debugMode") return m_debugMode;
    if (key == "background_color") return m_backgroundColor; // QColor jest automatycznie obsługiwany przez QVariant
    if (key == "blob_color") return m_blobColor;
    if (key == "stream_color") return m_streamColor;
    if (key == "grid_color") return m_gridColor;
    if (key == "grid_spacing") return m_gridSpacing;
    if (key == "title_text_color") return m_titleTextColor;
    if (key == "title_border_color") return m_titleBorderColor;
    if (key == "title_glow_color") return m_titleGlowColor;
    if (key == "recentColors") return m_recentColors; // QStringList jest obsługiwany przez QVariant

    // Jeśli klucz nie pasuje do żadnego znanego ustawienia
    qWarning() << "WavelengthConfig::getSetting - Unknown key:" << key;
    return {}; // Zwróć nieprawidłowy QVariant
}

QString WavelengthConfig::getPreferredStartFrequency() const {
    return m_preferredStartFrequency;
}

void WavelengthConfig::setPreferredStartFrequency(const QString &frequency) {
    bool ok;

    if (const double freqValue = frequency.toDouble(&ok); ok && freqValue >= 130.0) {
        if (const QString normalizedFrequency = QString::number(freqValue, 'f', 1); m_preferredStartFrequency != normalizedFrequency) {
            m_preferredStartFrequency = normalizedFrequency;
            emit configChanged("preferredStartFrequency");
            qDebug() << "Config: Preferred start frequency set to" << m_preferredStartFrequency;
        }
    } else {
        qWarning() << "Config: Invalid preferred start frequency provided:" << frequency << ". Keeping previous value:" << m_preferredStartFrequency;
        if (m_preferredStartFrequency.toDouble(&ok) < 130.0 || !ok) {
        m_preferredStartFrequency = QString::number(130.0, 'f', 1);
        emit configChanged("preferredStartFrequency");
        }
    }
}