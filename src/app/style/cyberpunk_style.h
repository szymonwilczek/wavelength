#ifndef CYBERPUNK_STYLE_H
#define CYBERPUNK_STYLE_H

#include <QColor>
#include <QString>

class CyberpunkStyle {
public:
    static QColor GetPrimaryColor() { return {0, 170, 255}; } // Neonowy niebieski
    static QColor GetSecondaryColor() { return {0, 220, 255}; } // Jaśniejszy niebieski
    static QColor GetAccentColor() { return {150, 70, 240}; } // Fioletowy akcent
    static QColor GetWarningColor() { return {255, 180, 0}; } // Ostrzegawczy żółty
    static QColor GetDangerColor() { return {255, 60, 60}; } // Niebezpieczny czerwony
    static QColor GetSuccessColor() { return {0, 240, 130}; } // Neonowy zielony

    static QColor GetBackgroundDarkColor() { return {10, 20, 30}; } // Bardzo ciemne tło
    static QColor GetBackgroundMediumColor() { return {20, 35, 50}; } // Średnie tło
    static QColor GetBackgroundLightColor() { return {30, 50, 70}; } // Jaśniejsze tło

    static QColor GetTextColor() { return {220, 230, 240}; } // Główny tekst
    static QColor GetMutedTextColor() { return {150, 160, 170}; } // Przytłumiony tekst

    static void ApplyStyle();

    static QString GetTechBorderStyle(bool is_active = true);

    static QString GetCyberpunkFrameStyle();
};

#endif // CYBERPUNK_STYLE_H
