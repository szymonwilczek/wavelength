#include "font_manager.h"
#include <QApplication>
#include <QDebug>

FontManager& FontManager::instance() {
    static FontManager instance;
    return instance;
}

FontManager::FontManager() {
    m_fontFamilyNames = {
        {FontFamily::Lato, "Lato"},
        {FontFamily::NotoSans, "Noto Sans Condensed"},
        {FontFamily::Poppins, "Poppins"}
    };

    m_fontStyleNames = {
        {FontStyle::Regular, "Regular"},
        {FontStyle::Bold, "Bold"},
        {FontStyle::Italic, "Italic"},
        {FontStyle::Light, "Light"},
        {FontStyle::Black, "Black"},
        {FontStyle::Thin, "Thin"},
        {FontStyle::BoldItalic, "Bold Italic"},
        {FontStyle::BlackItalic, "Black Italic"},
        {FontStyle::LightItalic, "Light Italic"},
        {FontStyle::ThinItalic, "Thin Italic"},
        {FontStyle::Medium, "Medium"},
        {FontStyle::SemiBold, "SemiBold"}
    };

    m_fontPaths = {
        {qMakePair(FontFamily::Lato, FontStyle::Regular), ":/assets/fonts/Lato/Lato-Regular.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::Bold), ":/assets/fonts/Lato/Lato-Bold.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::Italic), ":/assets/fonts/Lato/Lato-Italic.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::Light), ":/assets/fonts/Lato/Lato-Light.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::Black), ":/assets/fonts/Lato/Lato-Black.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::Thin), ":/assets/fonts/Lato/Lato-Thin.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::BoldItalic), ":/assets/fonts/Lato/Lato-BoldItalic.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::BlackItalic), ":/assets/fonts/Lato/Lato-BlackItalic.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::LightItalic), ":/assets/fonts/Lato/Lato-LightItalic.ttf"},
        {qMakePair(FontFamily::Lato, FontStyle::ThinItalic), ":/assets/fonts/Lato/Lato-ThinItalic.ttf"},

        {qMakePair(FontFamily::NotoSans, FontStyle::Regular), ":/assets/fonts/Noto_Sans/NotoSans_Condensed-Regular.ttf"},
        {qMakePair(FontFamily::NotoSans, FontStyle::Bold), ":/assets/fonts/Noto_Sans/NotoSans_Condensed-Bold.ttf"},
        {qMakePair(FontFamily::NotoSans, FontStyle::Black), ":/assets/fonts/Noto_Sans/NotoSans_Condensed-Black.ttf"},
        {qMakePair(FontFamily::NotoSans, FontStyle::Light), ":/assets/fonts/Noto_Sans/NotoSans_Condensed-Light.ttf"},
        {qMakePair(FontFamily::NotoSans, FontStyle::Medium), ":/assets/fonts/Noto_Sans/NotoSans_Condensed-Medium.ttf"},
        {qMakePair(FontFamily::NotoSans, FontStyle::SemiBold), ":/assets/fonts/Noto_Sans/NotoSans_Condensed-SemiBold.ttf"},

        {qMakePair(FontFamily::Poppins, FontStyle::Regular), ":/assets/fonts/Poppins/Poppins-Regular.ttf"},
        {qMakePair(FontFamily::Poppins, FontStyle::Bold), ":/assets/fonts/Poppins/Poppins-Bold.ttf"},
        {qMakePair(FontFamily::Poppins, FontStyle::Medium), ":/assets/fonts/Poppins/Poppins-Medium.ttf"},
        {qMakePair(FontFamily::Poppins, FontStyle::Light), ":/assets/fonts/Poppins/Poppins-Light.ttf"},
        {qMakePair(FontFamily::Poppins, FontStyle::SemiBold), ":/assets/fonts/Poppins/Poppins-SemiBold.ttf"}
    };

    m_loadedFonts[FontFamily::Lato] = false;
    m_loadedFonts[FontFamily::NotoSans] = false;
    m_loadedFonts[FontFamily::Poppins] = false;
}

bool FontManager::initialize() {
    bool allLoaded = true;
    
    for (auto it = m_fontPaths.begin(); it != m_fontPaths.end(); ++it) {
        FontFamily family = it.key().first;
        if (!loadFont(it.value())) {
            qWarning() << "Error loading font: " << it.value();
            allLoaded = false;
        } else {
            m_loadedFonts[family] = true;
        }
    }
    
    if (allLoaded) {
        qDebug() << "All fonts loaded successfully";
    } else {
        qWarning() << "Some fonts failed to load";
    }
    
    return allLoaded;
}

bool FontManager::loadFont(const QString& path) {
    int fontId = QFontDatabase::addApplicationFont(path);
    return fontId != -1;
}

QFont FontManager::getFont(FontFamily family, FontStyle style, int pointSize) {
    QString familyName = m_fontFamilyNames.value(family, "Lato");
    QString styleName = m_fontStyleNames.value(style, "Regular");
    
    QFont font(familyName, pointSize);
    
    switch (style) {
        case FontStyle::Bold:
        case FontStyle::BoldItalic:
            font.setWeight(QFont::Bold);
            break;
        case FontStyle::Light:
        case FontStyle::LightItalic:
            font.setWeight(QFont::Light);
            break;
        case FontStyle::Black:
        case FontStyle::BlackItalic:
            font.setWeight(QFont::Black);
            break;
        case FontStyle::Thin:
        case FontStyle::ThinItalic:
            font.setWeight(QFont::Thin);
            break;
        case FontStyle::Medium:
            font.setWeight(QFont::Medium);
            break;
        case FontStyle::SemiBold:
            font.setWeight(QFont::DemiBold);
            break;
        default:
            font.setWeight(QFont::Normal);
            break;
    }
    
    font.setItalic(style == FontStyle::Italic ||
                  style == FontStyle::BoldItalic || 
                  style == FontStyle::LightItalic || 
                  style == FontStyle::BlackItalic || 
                  style == FontStyle::ThinItalic);
    
    font.setStyleStrategy(QFont::PreferAntialias);
    
    return font;
}

void FontManager::setApplicationFont(FontFamily family, FontStyle style, int pointSize) {
    QFont defaultFont = getFont(family, style, pointSize);
    QApplication::setFont(defaultFont);
    qDebug() << "Set default font to:"
             << m_fontFamilyNames[family] << m_fontStyleNames[style] 
             << "with size " << pointSize;
}

bool FontManager::isFontLoaded(FontFamily family) const {
    return m_loadedFonts.value(family, false);
}

void FontManager::debugFontInfo() const {
    qDebug() << "== FONTS INFO ==";
    qDebug() << "Loaded fonts families:";
    
    for (auto it = m_loadedFonts.begin(); it != m_loadedFonts.end(); ++it) {
        QString familyName = m_fontFamilyNames.value(it.key());
        bool loaded = it.value();
        qDebug() << "  - " << familyName << ":" << (loaded ? "LOADED" : "FAILED TO LOAD");
    }
}