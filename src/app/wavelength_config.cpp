#include "wavelength_config.h"

#include <QCoreApplication>
#include <QDebug>

WavelengthConfig *WavelengthConfig::GetInstance() {
    if (!instance_) {
        instance_ = new WavelengthConfig();
    }
    return instance_;
}

WavelengthConfig::WavelengthConfig(QObject *parent)
    : QObject(parent),
      settings_(QSettings::UserScope,
                QCoreApplication::organizationName(), QCoreApplication::applicationName()) {
    LoadDefaultShortcuts();
    LoadDefaults();
    LoadSettings();
    qDebug() << "[CONFIG] Initialization complete.";
}

void WavelengthConfig::LoadDefaults() {
    relay_server_address_ = DefaultConfig::kRelayServerAddress;
    relay_server_port_ = DefaultConfig::kRelayServerPort;
    connection_timeout_ = DefaultConfig::kConnectionTimeout;
    keep_alive_interval_ = DefaultConfig::kKeepAliveInterval;
    max_reconnect_attempts_ = DefaultConfig::kMaxReconnectAttempts;
    debug_mode_ = DefaultConfig::kIsDebugMode;
    background_color_ = DefaultConfig::kBackgroundColor;
    blob_color_ = DefaultConfig::kBlobColor;
    stream_color_ = DefaultConfig::kStreamColor;
    recent_colors_.clear();

    grid_color_ = DefaultConfig::kGridColor;
    grid_spacing_ = DefaultConfig::kGridSpacing;
    title_text_color_ = DefaultConfig::kTitleTextColor;
    title_border_color_ = DefaultConfig::kTitleBorderColor;
    title_glow_color_ = DefaultConfig::kTitleGlowColor;
    preferred_start_frequency_ = "130.0";
    language_code_ = "en";
    shortcuts_ = default_shortcuts_;
}

void WavelengthConfig::LoadDefaultShortcuts() {
    default_shortcuts_.clear();
    // main window
    default_shortcuts_["MainWindow.CreateWavelength"] = QKeySequence("Ctrl+N");
    default_shortcuts_["MainWindow.JoinWavelength"] = QKeySequence("Ctrl+J");
    default_shortcuts_["MainWindow.OpenSettings"] = QKeySequence("Ctrl+,");
    // chat view
    default_shortcuts_["ChatView.AbortConnection"] = QKeySequence("Ctrl+Q");
    default_shortcuts_["ChatView.FocusInput"] = QKeySequence("Ctrl+L");
    default_shortcuts_["ChatView.AttachFile"] = QKeySequence("Ctrl+O");
    default_shortcuts_["ChatView.SendMessage"] = QKeySequence("Ctrl+Enter");
    default_shortcuts_["ChatView.TogglePTT"] = QKeySequence("Space");
    // settings view
    default_shortcuts_["SettingsView.SwitchTab0"] = QKeySequence("Ctrl+1");
    default_shortcuts_["SettingsView.SwitchTab1"] = QKeySequence("Ctrl+2");
    default_shortcuts_["SettingsView.SwitchTab2"] = QKeySequence("Ctrl+3");
    default_shortcuts_["SettingsView.SwitchTab3"] = QKeySequence("Ctrl+4");
    default_shortcuts_["SettingsView.SwitchTab4"] = QKeySequence("Ctrl+5");
    default_shortcuts_["SettingsView.Save"] = QKeySequence("Ctrl+S");
    default_shortcuts_["SettingsView.Defaults"] = QKeySequence("Ctrl+D");
    default_shortcuts_["SettingsView.Back"] = QKeySequence(Qt::Key_Escape);
}

void WavelengthConfig::LoadSettings() {
    settings_.beginGroup("Wavelength");
    preferred_start_frequency_ = settings_.value("preferredStartFrequency", preferred_start_frequency_).toDouble();
    settings_.endGroup();

    settings_.beginGroup("Application");
    debug_mode_ = settings_.value("debugMode", debug_mode_).toBool();
    language_code_ = settings_.value("languageCode", language_code_).toString();
    settings_.endGroup();

    settings_.beginGroup("Appearance");
    background_color_ = settings_.value("backgroundColor", background_color_).value<QColor>();
    blob_color_ = settings_.value("blobColor", blob_color_).value<QColor>();
    stream_color_ = settings_.value("streamColor", stream_color_).value<QColor>();
    recent_colors_ = settings_.value("recentColors", QStringList()).toStringList();
    grid_color_ = settings_.value("gridColor", grid_color_).value<QColor>();
    grid_spacing_ = settings_.value("gridSpacing", grid_spacing_).toInt();
    title_text_color_ = settings_.value("titleTextColor", title_text_color_).value<QColor>();
    title_border_color_ = settings_.value("titleBorderColor", title_border_color_).value<QColor>();
    title_glow_color_ = settings_.value("titleGlowColor", title_glow_color_).value<QColor>();
    settings_.endGroup();

    settings_.beginGroup("Network");
    relay_server_address_ = settings_.value("relayServerAddress", relay_server_address_).toString();
    relay_server_port_ = settings_.value("relayServerPort", relay_server_port_).toInt();
    connection_timeout_ = settings_.value("connectionTimeout", connection_timeout_).toInt();
    keep_alive_interval_ = settings_.value("keepAliveInterval", keep_alive_interval_).toInt();
    max_reconnect_attempts_ = settings_.value("maxReconnectAttempts", max_reconnect_attempts_).toInt();
    settings_.endGroup();

    settings_.beginGroup(kShortcutsPrefix);
    QStringList shortcut_keys = settings_.childKeys();
    for (const QString &key: shortcut_keys) {
        auto sequence = settings_.value(key).value<QKeySequence>();
        if (!sequence.isEmpty()) {
            QString action_id = key;
            shortcuts_[action_id] = sequence;
        } else {
            qDebug() << "[CONFIG] Warning: Empty shortcut found in settings for key:" << key;
            if (default_shortcuts_.contains(key) && !shortcuts_.contains(key)) {
                shortcuts_[key] = default_shortcuts_.value(key);
            }
        }
    }
    settings_.endGroup();
}

void WavelengthConfig::SaveSettings() {
    settings_.beginGroup("Wavelength");
    settings_.setValue("preferredStartFrequency", preferred_start_frequency_);
    settings_.endGroup();

    settings_.beginGroup("Application");
    settings_.setValue("debugMode", debug_mode_);
    settings_.setValue("languageCode", language_code_);
    settings_.endGroup();

    settings_.beginGroup("Appearance");
    settings_.setValue("backgroundColor", background_color_);
    settings_.setValue("blobColor", blob_color_);
    settings_.setValue("streamColor", stream_color_);
    settings_.setValue("recentColors", recent_colors_);
    settings_.setValue("gridColor", grid_color_);
    settings_.setValue("gridSpacing", grid_spacing_);
    settings_.setValue("titleTextColor", title_text_color_);
    settings_.setValue("titleBorderColor", title_border_color_);
    settings_.setValue("titleGlowColor", title_glow_color_);
    settings_.endGroup();

    settings_.beginGroup("Network");
    settings_.setValue("relayServerAddress", relay_server_address_);
    settings_.setValue("relayServerPort", relay_server_port_);
    settings_.setValue("connectionTimeout", connection_timeout_);
    settings_.setValue("keepAliveInterval", keep_alive_interval_);
    settings_.setValue("maxReconnectAttempts", max_reconnect_attempts_);
    settings_.endGroup();

    settings_.beginGroup(kShortcutsPrefix);
    settings_.remove("");

    for (auto it = shortcuts_.constBegin(); it != shortcuts_.constEnd(); ++it) {
        QString action_id = it.key();
        QKeySequence sequence = it.value();
        settings_.setValue(action_id, sequence);
    }

    settings_.endGroup();
    settings_.sync();
}

void WavelengthConfig::RestoreDefaults() {
    LoadDefaults();
    settings_.beginGroup(kShortcutsPrefix);
    settings_.remove(""); // removes all key in shortcuts group
    settings_.endGroup();
    SaveSettings();
    emit configChanged("all");
    emit recentColorsChanged();
    qDebug() << "[CONFIG] Restored default settings.";
}

QString WavelengthConfig::GetRelayServerAddress() const { return relay_server_address_; }
int WavelengthConfig::GetRelayServerPort() const { return relay_server_port_; }
QColor WavelengthConfig::GetBackgroundColor() const { return background_color_; }
QColor WavelengthConfig::GetBlobColor() const { return blob_color_; }
QColor WavelengthConfig::GetStreamColor() const { return stream_color_; }
QStringList WavelengthConfig::GetRecentColors() const { return recent_colors_; }
QColor WavelengthConfig::GetGridColor() const { return grid_color_; }
int WavelengthConfig::GetGridSpacing() const { return grid_spacing_; }
QColor WavelengthConfig::GetTitleTextColor() const { return title_text_color_; }
QColor WavelengthConfig::GetTitleBorderColor() const { return title_border_color_; }
QColor WavelengthConfig::GetTitleGlowColor() const { return title_glow_color_; }
QString WavelengthConfig::GetLanguageCode() const { return language_code_; }
int WavelengthConfig::GetConnectionTimeout() const { return connection_timeout_; }
int WavelengthConfig::GetKeepAliveInterval() const { return keep_alive_interval_; }
int WavelengthConfig::GetMaxReconnectAttempts() const { return max_reconnect_attempts_; }
bool WavelengthConfig::IsDebugMode() const { return debug_mode_; }
QMap<QString, QKeySequence> WavelengthConfig::GetAllShortcuts() const { return shortcuts_; }
QMap<QString, QKeySequence> WavelengthConfig::GetDefaultShortcutsMap() const { return default_shortcuts_; }
QString WavelengthConfig::GetPreferredStartFrequency() const { return preferred_start_frequency_; }

QString WavelengthConfig::GetRelayServerUrl() const {
    return QString("ws://%1:%2").arg(relay_server_address_).arg(relay_server_port_);
}

QKeySequence WavelengthConfig::GetShortcut(const QString &action_id, const QKeySequence &default_sequence) const {
    const QKeySequence actual_default = default_sequence.isEmpty()
                                            ? default_shortcuts_.value(action_id)
                                            : default_sequence;
    QKeySequence result = shortcuts_.value(action_id, actual_default);
    return result;
}

void WavelengthConfig::SetLanguageCode(const QString &code) {
    if (language_code_ != code && !code.isEmpty()) {
        language_code_ = code;
        emit configChanged("languageCode");
    }
}

void WavelengthConfig::SetRelayServerAddress(const QString &address) {
    if (relay_server_address_ != address) {
        relay_server_address_ = address;
        emit configChanged("relayServerAddress");
    }
}

void WavelengthConfig::SetRelayServerPort(const int port) {
    if (relay_server_port_ != port) {
        relay_server_port_ = port;
        emit configChanged("relayServerPort");
    }
}

void WavelengthConfig::SetBackgroundColor(const QColor &color) {
    if (background_color_ != color && color.isValid()) {
        background_color_ = color;
        AddRecentColor(color);
        emit configChanged("background_color");
    }
}

void WavelengthConfig::SetBlobColor(const QColor &color) {
    if (blob_color_ != color && color.isValid()) {
        blob_color_ = color;
        AddRecentColor(color);
        emit configChanged("blob_color");
    }
}

void WavelengthConfig::SetStreamColor(const QColor &color) {
    if (stream_color_ != color && color.isValid()) {
        stream_color_ = color;
        AddRecentColor(color);
        emit configChanged("stream_color");
    }
}

void WavelengthConfig::SetGridColor(const QColor &color) {
    if (grid_color_ != color && color.isValid()) {
        grid_color_ = color;
        AddRecentColor(color);
        emit configChanged("grid_color");
    }
}

void WavelengthConfig::SetGridSpacing(const int spacing) {
    if (grid_spacing_ != spacing && spacing > 0) {
        grid_spacing_ = spacing;
        emit configChanged("grid_spacing");
    }
}

void WavelengthConfig::SetTitleTextColor(const QColor &color) {
    if (title_text_color_ != color && color.isValid()) {
        title_text_color_ = color;
        AddRecentColor(color);
        emit configChanged("title_text_color");
    }
}

void WavelengthConfig::SetTitleBorderColor(const QColor &color) {
    if (title_border_color_ != color && color.isValid()) {
        title_border_color_ = color;
        AddRecentColor(color);
        emit configChanged("title_border_color");
    }
}

void WavelengthConfig::SetTitleGlowColor(const QColor &color) {
    if (title_glow_color_ != color && color.isValid()) {
        title_glow_color_ = color;
        AddRecentColor(color);
        emit configChanged("title_glow_color");
    }
}

void WavelengthConfig::AddRecentColor(const QColor &color) {
    if (!color.isValid()) return;

    const QString hex = color.name(QColor::HexRgb);

    recent_colors_.removeAll(hex);
    recent_colors_.prepend(hex);

    while (recent_colors_.size() > kMaxRecentColors) {
        recent_colors_.removeLast();
    }
    emit recentColorsChanged();
}

void WavelengthConfig::SetConnectionTimeout(const int timeout) {
    if (connection_timeout_ != timeout && timeout > 0) {
        connection_timeout_ = timeout;
        emit configChanged("connectionTimeout");
    }
}

void WavelengthConfig::SetKeepAliveInterval(const int interval) {
    if (keep_alive_interval_ != interval && interval > 0) {
        keep_alive_interval_ = interval;
        emit configChanged("keepAliveInterval");
    }
}

void WavelengthConfig::SetMaxReconnectAttempts(const int attempts) {
    if (max_reconnect_attempts_ != attempts && attempts >= 0) {
        max_reconnect_attempts_ = attempts;
        emit configChanged("maxReconnectAttempts");
    }
}

void WavelengthConfig::SetDebugMode(const bool enabled) {
    if (debug_mode_ != enabled) {
        debug_mode_ = enabled;
        emit configChanged("debugMode");
    }
}

void WavelengthConfig::SetShortcut(const QString &action_id, const QKeySequence &sequence) {
    if (shortcuts_.value(action_id) != sequence) {
        shortcuts_[action_id] = sequence;
    }
}

QVariant WavelengthConfig::GetSetting(const QString &key) const {
    if (key == "relayServerAddress") return relay_server_address_;
    if (key == "relayServerPort") return relay_server_port_;
    if (key == "connectionTimeout") return connection_timeout_;
    if (key == "keepAliveInterval") return keep_alive_interval_;
    if (key == "maxReconnectAttempts") return max_reconnect_attempts_;
    if (key == "debugMode") return debug_mode_;
    if (key == "background_color") return background_color_;
    if (key == "blob_color") return blob_color_;
    if (key == "stream_color") return stream_color_;
    if (key == "grid_color") return grid_color_;
    if (key == "grid_spacing") return grid_spacing_;
    if (key == "title_text_color") return title_text_color_;
    if (key == "title_border_color") return title_border_color_;
    if (key == "title_glow_color") return title_glow_color_;
    if (key == "recentColors") return recent_colors_;
    if (key == "languageCode") return language_code_;

    qWarning() << "[CONFIG] Unknown key:" << key;
    return {};
}

void WavelengthConfig::SetPreferredStartFrequency(const QString &frequency) {
    bool ok;

    if (const double frequency_value = frequency.toDouble(&ok); ok && frequency_value >= 130.0) {
        if (const QString normalized_frequency = QString::number(frequency_value, 'f', 1);
            preferred_start_frequency_ != normalized_frequency) {
            preferred_start_frequency_ = normalized_frequency;
            emit configChanged("preferredStartFrequency");
        }
    } else {
        qWarning() << "[CONFIG] Invalid preferred start frequency provided:" << frequency << ". Keeping previous value:"
                << preferred_start_frequency_;
        if (preferred_start_frequency_.toDouble(&ok) < 130.0 || !ok) {
            preferred_start_frequency_ = QString::number(130.0, 'f', 1);
            emit configChanged("preferredStartFrequency");
        }
    }
}

WavelengthConfig *WavelengthConfig::instance_ = nullptr;
