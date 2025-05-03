#include "font_manager.h"
#include <QFontDatabase>
#include <QDebug>
#include <QDirIterator>
#include <QCoreApplication>

FontManager* FontManager::instance_ = nullptr;

FontManager* FontManager::GetInstance() {
    if (instance_ == nullptr) {
        instance_ = new FontManager();
    }
    return instance_;
}

FontManager::FontManager(QObject* parent) : QObject(parent) {
}

bool FontManager::Initialize() {
    bool success = true;
    
    QStringList base_paths = {
        ":/resources/fonts/",
        QCoreApplication::applicationDirPath() + "/../resources/fonts/",
        QCoreApplication::applicationDirPath() + "/../../resources/fonts/"
    };
    
    QMap<QString, QString> font_directories = {
        {"BlenderPro", "BlenderPro"},
        {"Lato", "Lato"},
        {"Poppins", "Poppins"}
    };
    
    for (const auto& fontName : font_directories.keys()) {
        bool font_loaded = false;
        QString directory_name = font_directories[fontName];
        
        for (const QString& base_path : base_paths) {
            QString font_directory = base_path + directory_name + "/";
            QDirIterator it(font_directory, QStringList() << "*.ttf", QDir::Files);
            
            while (it.hasNext()) {
                if (QString font_path = it.next(); LoadFont(fontName, font_path)) {
                    font_loaded = true;
                }
            }
            
            if (font_loaded) {
                break;
            }
        }
        
        if (!font_loaded) {
            qWarning() << "Nie udało się załadować czcionki:" << fontName;
            success = false;
        }
    }
    
    return success;
}

bool FontManager::LoadFont(const QString& font_name, const QString& font_path) {
    const int font_id = QFontDatabase::addApplicationFont(font_path);
    if (font_id == -1) {
        qWarning() << "Błąd ładowania czcionki:" << font_path;
        return false;
    }
    
    QStringList families = QFontDatabase::applicationFontFamilies(font_id);
    if (families.isEmpty()) {
        qWarning() << "Brak dostępnych rodzin czcionek dla:" << font_path;
        return false;
    }

    const QString& family = families.first();
    font_families_[font_name] = family;
    return true;
}

QString FontManager::GetFontFamily(const QString& font_name) {
    if (font_families_.contains(font_name)) {
        return font_families_[font_name];
    }
    
    qWarning() << "Nie znaleziono rodziny czcionki:" << font_name << "- używam domyślnej";
    return QFont().family();
}

QFont FontManager::GetFont(const QString& font_name, const int point_size, const int weight) {
    const QString family = GetFontFamily(font_name);
    QFont font(family, point_size);
    font.setWeight(weight);
    return font;
}