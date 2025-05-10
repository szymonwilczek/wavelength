#ifndef CYBERPUNK_STYLE_H
#define CYBERPUNK_STYLE_H

#include <QColor>
#include <QString>

/**
 * @brief Provides a static interface for accessing cyberpunk-themed colors and applying a global stylesheet.
 *
 * This class defines a set of predefined QColor values representing a cyberpunk color palette.
 * It also includes a method to apply a comprehensive stylesheet to the entire QApplication,
 * using these colors to style various standard Qt widgets (buttons, line edits, scrollbars, etc.).
 * Additionally, it offers helper methods to generate specific CSS-like style strings for custom widgets.
 */
class CyberpunkStyle {
public:
    /**
     * @brief Applies the cyberpunk style globally to the QApplication.
     * Sets the application style to "Fusion", configures a custom QPalette using the defined colors,
     * and applies a detailed stylesheet to style common widgets.
     */
    static void ApplyStyle();

    /**
     * @brief Private constructor to prevent instantiation. This class only provides static members.
     */
    CyberpunkStyle() = delete;

    /**
     * @brief Gets the primary color (Neon Blue).
     * @return QColor(0, 170, 255).
     */
    static QColor GetPrimaryColor() { return {0, 170, 255}; } // Neonowy niebieski

    /**
     * @brief Gets the secondary color (Lighter Blue).
     * @return QColor(0, 220, 255).
     */
    static QColor GetSecondaryColor() { return {0, 220, 255}; } // Jaśniejszy niebieski

    /**
     * @brief Gets the accent color (Purple).
     * @return QColor(150, 70, 240).
     */
    static QColor GetAccentColor() { return {150, 70, 240}; } // Fioletowy akcent

    /**
     * @brief Gets the warning color (Warning Yellow).
     * @return QColor(255, 180, 0).
     */
    static QColor GetWarningColor() { return {255, 180, 0}; } // Ostrzegawczy żółty

    /**
     * @brief Gets the danger color (Danger Red).
     * @return QColor(255, 60, 60).
     */
    static QColor GetDangerColor() { return {255, 60, 60}; } // Niebezpieczny czerwony

    /**
     * @brief Gets the success color (Neon Green).
     * @return QColor(0, 240, 130).
     */
    static QColor GetSuccessColor() { return {0, 240, 130}; } // Neonowy zielony

    /**
     * @brief Gets the dark background color.
     * @return QColor(10, 20, 30).
     */
    static QColor GetBackgroundDarkColor() { return {10, 20, 30}; } // Bardzo ciemne tło

    /**
     * @brief Gets the medium background color.
     * @return QColor(20, 35, 50).
     */
    static QColor GetBackgroundMediumColor() { return {20, 35, 50}; } // Średnie tło

    /**
     * @brief Gets the light background color.
     * @return QColor(30, 50, 70).
     */
    static QColor GetBackgroundLightColor() { return {30, 50, 70}; } // Jaśniejsze tło

    /**
     * @brief Gets the main text color.
     * @return QColor(220, 230, 240).
     */
    static QColor GetTextColor() { return {220, 230, 240}; } // Główny tekst

    /**
     * @brief Gets the muted text color (for disabled elements, etc.).
     * @return QColor(150, 160, 170).
     */
    static QColor GetMutedTextColor() { return {150, 160, 170}; } // Przytłumiony tekst


    /**
     * @brief Generates a CSS-like border style string with a "tech" look.
     * The border color depends on the `is_active` flag.
     * @param is_active If true, uses the primary color; otherwise, uses a darker version. Defaults to true.
     * @return A QString containing the border style definition (e.g., "border: 1px solid #00aaff; border-radius: 0px;").
     */
    static QString GetTechBorderStyle(bool is_active = true);

    /**
     * @brief Generates a CSS-like style string for a basic cyberpunk-themed frame.
     * Includes background color, border, and border-radius.
     * @return A QString containing the frame style definition.
     */
    static QString GetCyberpunkFrameStyle();
};

#endif // CYBERPUNK_STYLE_H
