#include "font_manager.h"
#include <QFontDatabase>
#include <QDebug>
#include <QDirIterator>
#include <QCoreApplication>

FontManager* FontManager::instance = nullptr;

FontManager* FontManager::getInstance() {
    if (instance == nullptr) {
        instance = new FontManager();
    }
    return instance;
}

FontManager::FontManager(QObject* parent) : QObject(parent) {
}

bool FontManager::initialize() {
    bool success = true;
    
    // Podstawowe ścieżki do sprawdzenia
    QStringList basePaths = {
        ":/assets/fonts/",                                   // ścieżka do czcionek w zasobach
        QCoreApplication::applicationDirPath() + "/../assets/fonts/",  // względna ścieżka dla trybu release
        QCoreApplication::applicationDirPath() + "/../../assets/fonts/" // względna ścieżka dla trybu debug
    };
    
    // Lista czcionek do załadowania z ich katalogami
    QMap<QString, QString> fontDirs = {
        {"BlenderPro", "BlenderPro"},
        {"Lato", "Lato"},
        {"Poppins", "Poppins"}
    };
    
    for (const auto& fontName : fontDirs.keys()) {
        bool fontLoaded = false;
        QString dirName = fontDirs[fontName];
        
        for (const QString& basePath : basePaths) {
            QString fontDir = basePath + dirName + "/";
            QDirIterator it(fontDir, QStringList() << "*.ttf", QDir::Files);
            
            while (it.hasNext()) {
                QString fontPath = it.next();
                if (loadFont(fontName, fontPath)) {
                    qDebug() << "Załadowano czcionkę:" << fontPath;
                    fontLoaded = true;
                    // Nie przerywamy, żeby załadować wszystkie warianty (.ttf) danej czcionki
                }
            }
            
            if (fontLoaded) {
                break; // Jeśli znaleźliśmy i załadowaliśmy czcionkę w tej ścieżce, nie sprawdzamy pozostałych
            }
        }
        
        if (!fontLoaded) {
            qWarning() << "Nie udało się załadować czcionki:" << fontName;
            success = false;
        }
    }
    
    return success;
}

bool FontManager::loadFont(const QString& fontName, const QString& fontPath) {
    int fontId = QFontDatabase::addApplicationFont(fontPath);
    if (fontId == -1) {
        qWarning() << "Błąd ładowania czcionki:" << fontPath;
        return false;
    }
    
    QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        qWarning() << "Brak dostępnych rodzin czcionek dla:" << fontPath;
        return false;
    }
    
    QString family = families.first();
    fontFamilies[fontName] = family;
    qDebug() << "Załadowana rodzina czcionki:" << fontName << "jako" << family;
    return true;
}

QString FontManager::getFontFamily(const QString& fontName) {
    if (fontFamilies.contains(fontName)) {
        return fontFamilies[fontName];
    }
    
    qWarning() << "Nie znaleziono rodziny czcionki:" << fontName << "- używam domyślnej";
    return QFont().family(); // Zwróć domyślną czcionkę systemową
}

QFont FontManager::getFont(const QString& fontName, int pointSize, int weight) {
    QString family = getFontFamily(fontName);
    QFont font(family, pointSize);
    font.setWeight(weight);
    return font;
}