#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QFont>

class FontManager : public QObject {
    Q_OBJECT

public:
    static FontManager* getInstance();

    // Inicjalizacja i ładowanie wszystkich czcionek
    bool initialize();

    // Pobieranie nazwy rodziny czcionki dla danego stylu
    QString getFontFamily(const QString& fontName);
    
    // Pobieranie czcionki z daną rodziną, rozmiarem i wagą
    QFont getFont(const QString& fontName, int pointSize = 10, int weight = QFont::Normal);

private:
    FontManager(QObject* parent = nullptr);
    ~FontManager() = default;

    static FontManager* instance;

    // Mapa przechowująca nazwy załadowanych rodzin czcionek
    QMap<QString, QString> fontFamilies;

    // Funkcja ładująca pojedynczą czcionkę
    bool loadFont(const QString& fontName, const QString& fontPath);
};

#endif // FONT_MANAGER_H