#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QFont>
#include <QFontDatabase>
#include <QMap>
#include <QString>

enum class FontFamily {
    Lato,
    NotoSans,
    Poppins
};

enum class FontStyle {
    Regular,
    Bold,
    Italic,
    Light,
    Black,
    Thin,
    BoldItalic,
    BlackItalic,
    LightItalic,
    ThinItalic,
    Medium,
    SemiBold
};

class FontManager {
public:
    static FontManager& instance();
    
    bool initialize();
    
    QFont getFont(FontFamily family = FontFamily::Lato,
                  FontStyle style = FontStyle::Regular, 
                  int pointSize = 10);
                  
    void setApplicationFont(FontFamily family = FontFamily::Lato,
                           FontStyle style = FontStyle::Regular, 
                           int pointSize = 10);
                           
    bool isFontLoaded(FontFamily family) const;
    
    void debugFontInfo() const;

private:
    FontManager();
    
    bool loadFont(const QString& path);
    
    QMap<FontFamily, QString> m_fontFamilyNames;
    
    QMap<FontStyle, QString> m_fontStyleNames;
    
    QMap<QPair<FontFamily, FontStyle>, QString> m_fontPaths;
    
    QMap<FontFamily, bool> m_loadedFonts;
};

#endif // FONT_MANAGER_H