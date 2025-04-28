#ifndef CYBERPUNK_STYLE_H
#define CYBERPUNK_STYLE_H

#include <QColor>
#include <QString>

class CyberpunkStyle {
public:
    static QColor primaryColor() { return {0, 170, 255}; } // Neonowy niebieski
    static QColor secondaryColor() { return {0, 220, 255}; } // Jaśniejszy niebieski
    static QColor accentColor() { return {150, 70, 240}; } // Fioletowy akcent
    static QColor warningColor() { return {255, 180, 0}; } // Ostrzegawczy żółty
    static QColor dangerColor() { return {255, 60, 60}; } // Niebezpieczny czerwony
    static QColor successColor() { return {0, 240, 130}; } // Neonowy zielony

    static QColor bgDarkColor() { return {10, 20, 30}; } // Bardzo ciemne tło
    static QColor bgMediumColor() { return {20, 35, 50}; } // Średnie tło
    static QColor bgLightColor() { return {30, 50, 70}; } // Jaśniejsze tło

    static QColor textColor() { return {220, 230, 240}; } // Główny tekst
    static QColor mutedTextColor() { return {150, 160, 170}; } // Przytłumiony tekst

    static void applyStyle();

    static QString getTechBorderStyle(bool isActive = true);

    static QString getCyberpunkFrameStyle();
};

#endif // CYBERPUNK_STYLE_H
