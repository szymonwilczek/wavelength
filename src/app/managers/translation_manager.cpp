#include "translation_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QMutexLocker>

TranslationManager* TranslationManager::instance_ = nullptr;
QMutex TranslationManager::mutex_;

TranslationManager* TranslationManager::GetInstance() {
    if (!instance_) {
        QMutexLocker locker(&mutex_); // Zabezpieczenie przed wyścigiem przy tworzeniu
        if (!instance_) {
            instance_ = new TranslationManager();
        }
    }
    return instance_;
}

TranslationManager::TranslationManager(QObject *parent) : QObject(parent) {}

bool TranslationManager::Initialize(const QString& language_code) {
    if (initialized_) {
        qWarning() << "TranslationManager already initialized.";
        // Można rozważyć ponowne załadowanie, jeśli język się zmienił,
        // ale zgodnie z wymaganiem restartu, nie jest to konieczne tutaj.
        return true;
    }

    qInfo() << "Initializing TranslationManager with language:" << language_code;
    initialized_ = LoadTranslations(language_code);
    if (!initialized_) {
        qCritical() << "Failed to load translations for" << language_code << ". Falling back to English.";
        // Spróbuj załadować angielski jako fallback
        initialized_ = LoadTranslations("en");
        if (!initialized_) {
            qCritical() << "Failed to load English translations as fallback. Translation system unavailable.";
        }
    }
    return initialized_;
}

bool TranslationManager::LoadTranslations(const QString& language_code) {
    QString file_path = QString(":/translations/%1.json").arg(language_code);
    QFile file(file_path);

    if (!file.exists()) {
        qWarning() << "Translation file not found:" << file_path;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open translation file:" << file_path << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);

    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse translation file:" << file_path << "Error:" << parse_error.errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "Translation file does not contain a root JSON object:" << file_path;
        return false;
    }

    translations_ = doc.object();
    current_language_code_ = language_code;
    qInfo() << "Successfully loaded translations from:" << file_path;
    return true;
}

QString TranslationManager::Translate(const QString& key, const QString& default_value) const {
    if (!initialized_ || translations_.isEmpty()) {
        // qWarning() << "TranslationManager not initialized or no translations loaded. Key:" << key;
        return default_value.isEmpty() ? key : default_value; // Zwróć klucz lub domyślną wartość
    }

    QStringList key_parts = key.split('.');
    QJsonObject current_obj = translations_;
    QJsonValue current_val;

    for (int i = 0; i < key_parts.size(); ++i) {
        const QString& part = key_parts[i];
        if (!current_obj.contains(part)) {
            // qWarning() << "Translation key not found:" << key << "(missing part:" << part << ")";
            return default_value.isEmpty() ? key : default_value;
        }

        current_val = current_obj.value(part);
        if (i < key_parts.size() - 1) {
            if (!current_val.isObject()) {
                // qWarning() << "Invalid translation structure for key:" << key << "(part:" << part << "is not an object)";
                return default_value.isEmpty() ? key : default_value;
            }
            current_obj = current_val.toObject();
        }
    }

    if (current_val.isString()) {
        return current_val.toString();
    }

    // qWarning() << "Translation value is not a string for key:" << key;
    return default_value.isEmpty() ? key : default_value;
}

QString TranslationManager::GetCurrentLanguage() const {
    return current_language_code_;
}