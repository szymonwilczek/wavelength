#include "translation_manager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QMutexLocker>

bool TranslationManager::Initialize(const QString &language_code) {
    if (initialized_) {
        qWarning() << "[TRANSLATION MANAGER] Warning: Already initialized.";
        return true;
    }

    qInfo() << "[TRANSLATION MANAGER] Initializing with language:" << language_code;
    initialized_ = LoadTranslations(language_code);
    if (!initialized_) {
        qCritical() << "[TRANSLATION MANAGER] Failed to load translations for" << language_code <<
                ". Falling back to English.";
        initialized_ = LoadTranslations("en");
        if (!initialized_) {
            qCritical() <<
                    "[TRANSLATION MANAGER] Failed to load English translations as fallback. Translation system unavailable.";
        }
    }
    return initialized_;
}

TranslationManager *TranslationManager::GetInstance() {
    if (!instance_) {
        QMutexLocker locker(&mutex_);
        instance_ = new TranslationManager();
    }
    return instance_;
}

QString TranslationManager::Translate(const QString &key, const QString &default_value) const {
    if (!initialized_ || translations_.isEmpty()) {
        qWarning() << "[TRANSLATION MANAGER] Not initialized or no translations loaded. Key:" << key;
        return default_value.isEmpty() ? key : default_value;
    }

    QStringList key_parts = key.split('.');
    QJsonObject current_obj = translations_;
    QJsonValue current_val;

    for (int i = 0; i < key_parts.size(); ++i) {
        const QString &part = key_parts[i];
        if (!current_obj.contains(part)) {
            qWarning() << "[TRANSLATION MANAGER] Translation key not found:" << key << "(missing part:" << part << ")";
            return default_value.isEmpty() ? key : default_value;
        }

        current_val = current_obj.value(part);
        if (i < key_parts.size() - 1) {
            if (!current_val.isObject()) {
                qWarning() << "[TRANSLATION MANAGER] Invalid translation structure for key:" << key << "(part:" << part
                        << "is not an object)";
                return default_value.isEmpty() ? key : default_value;
            }
            current_obj = current_val.toObject();
        }
    }

    if (current_val.isString()) {
        return current_val.toString();
    }

    return default_value.isEmpty() ? key : default_value;
}

QString TranslationManager::GetCurrentLanguage() const {
    return current_language_code_;
}

bool TranslationManager::LoadTranslations(const QString &language_code) {
    const QString file_path = QString(":/translations/%1.json").arg(language_code);
    QFile file(file_path);

    if (!file.exists()) {
        qWarning() << "[TRANSLATION MANAGER] Translation file not found:" << file_path;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[TRANSLATION MANAGER] Could not open translation file:" << file_path << file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parse_error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);

    if (parse_error.error != QJsonParseError::NoError) {
        qWarning() << "[TRANSLATION MANAGER] Failed to parse translation file:" << file_path << "Error:" << parse_error.
                errorString();
        return false;
    }

    if (!doc.isObject()) {
        qWarning() << "[TRANSLATION MANAGER] Translation file does not contain a root JSON object:" << file_path;
        return false;
    }

    translations_ = doc.object();
    current_language_code_ = language_code;
    qInfo() << "[TRANSLATION MANAGER] Successfully loaded translations from:" << file_path;
    return true;
}

TranslationManager *TranslationManager::instance_ = nullptr;
QMutex TranslationManager::mutex_;
