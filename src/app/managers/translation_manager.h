#ifndef TRANSLATION_MANAGER_H
#define TRANSLATION_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QMutex>

/**
 * @brief Manages the loading and delivery of translations for applications.
 *
 * The class implements the singleton pattern and is responsible for loading
 * the corresponding JSON file with translations based on the selected language
 * and providing a method to retrieve the translated strings.
 */
class TranslationManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Initializes the translation manager by loading the appropriate language file.
     * @param language_code Language code (e.g., "en", "pl").
     * @return True if the initialization and loading of translations was successful, false otherwise.
     */
    bool Initialize(const QString &language_code);

    /**
     * @brief Returns an instance of the TranslationManager singleton.
     * @return A pointer to an instance of TranslationManager.
     */
    static TranslationManager *GetInstance();

    /**
     * @brief Retrieves the translated string for the specified key.
     *
     * The key should have the format “Class.Widget.Property” or similar,
     * corresponding to the structure in the JSON file.
     *
     * @param key Translation key.
     * @param default_value The value returned if the key is not found.
     * @return Translated string or default_value.
     */
    QString Translate(const QString &key, const QString &default_value = QString()) const;

    /**
     * @brief Returns the currently loaded language code.
     * @return Language code (e.g. "en", "pl").
     */
    QString GetCurrentLanguage() const;

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional QObject parent.
     */
    explicit TranslationManager(QObject *parent = nullptr) : QObject(parent) {
    }

    /** @brief Removed copy constructor. */
    TranslationManager(const TranslationManager &) = delete;

    /** @brief Removed assignment constructor. */
    TranslationManager &operator=(const TranslationManager &) = delete;

    /**
     * @brief Loads translations from a JSON file for the specified language code.
     * @param language_code Language code to be loaded.
     * @return True if loading is successful, false otherwise.
     */
    bool LoadTranslations(const QString &language_code);

    /** @brief Static singleton instance. */
    static TranslationManager *instance_;
    /** @brief Mutex to protect instance creation in a multithreaded environment. */
    static QMutex mutex_;

    /** @brief It stores the loaded translations as a JSON object. */
    QJsonObject translations_;
    /** @brief Currently loaded language code. */
    QString current_language_code_;
    /** @brief A flag indicating whether the manager has been initialized. */
    bool initialized_ = false;
};

#endif // TRANSLATION_MANAGER_H
