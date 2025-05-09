#ifndef TRANSLATION_MANAGER_H
#define TRANSLATION_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QMutex>

/**
 * @brief Zarządza ładowaniem i dostarczaniem tłumaczeń dla aplikacji.
 *
 * Klasa implementuje wzorzec singleton i jest odpowiedzialna za wczytanie
 * odpowiedniego pliku JSON z tłumaczeniami na podstawie wybranego języka
 * oraz udostępnianie metody do pobierania przetłumaczonych ciągów znaków.
 */
class TranslationManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Zwraca instancję singletona TranslationManager.
     * @return Wskaźnik do instancji TranslationManager.
     */
    static TranslationManager* GetInstance();

    /**
     * @brief Inicjalizuje menedżera tłumaczeń, ładując odpowiedni plik językowy.
     * @param language_code Kod języka (np. "en", "pl").
     * @return True, jeśli inicjalizacja i załadowanie tłumaczeń powiodło się, false w przeciwnym razie.
     */
    bool Initialize(const QString& language_code);

    /**
     * @brief Pobiera przetłumaczony ciąg znaków dla podanego klucza.
     *
     * Klucz powinien mieć format "Klasa.Widget.Właściwość" lub podobny,
     * odpowiadający strukturze w pliku JSON.
     *
     * @param key Klucz tłumaczenia.
     * @param default_value Wartość zwracana, jeśli klucz nie zostanie znaleziony.
     * @return Przetłumaczony ciąg znaków lub default_value.
     */
    QString Translate(const QString& key, const QString& default_value = QString()) const;

    /**
     * @brief Zwraca aktualnie załadowany kod języka.
     * @return Kod języka (np. "en", "pl").
     */
    QString GetCurrentLanguage() const;

private:
    /**
     * @brief Prywatny konstruktor, aby wymusić wzorzec singleton.
     * @param parent Opcjonalny rodzic QObject.
     */
    explicit TranslationManager(QObject *parent = nullptr);

    /** @brief Usunięty konstruktor kopiujący. */
    TranslationManager(const TranslationManager&) = delete;
    /** @brief Usunięty operator przypisania. */
    TranslationManager& operator=(const TranslationManager&) = delete;

    /**
     * @brief Ładuje tłumaczenia z pliku JSON dla podanego kodu języka.
     * @param language_code Kod języka do załadowania.
     * @return True, jeśli ładowanie powiodło się, false w przeciwnym razie.
     */
    bool LoadTranslations(const QString& language_code);

    /** @brief Statyczna instancja singletona. */
    static TranslationManager* instance_;
    /** @brief Mutex do ochrony tworzenia instancji w środowisku wielowątkowym. */
    static QMutex mutex_;

    /** @brief Przechowuje załadowane tłumaczenia jako obiekt JSON. */
    QJsonObject translations_;
    /** @brief Aktualnie załadowany kod języka. */
    QString current_language_code_;
    /** @brief Flaga wskazująca, czy menedżer został zainicjalizowany. */
    bool initialized_ = false;
};

#endif // TRANSLATION_MANAGER_H