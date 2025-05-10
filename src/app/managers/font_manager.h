#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QFont>

/**
 * @brief Manages application fonts using a singleton pattern.
 *
 * This class is responsible for loading fonts from specified directories
 * and providing access to them throughout the application. It ensures that
 * fonts are loaded only once.
 */
class FontManager final : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Initializes the FontManager by loading fonts from predefined paths.
     *
     * Searches for font files (.ttf) within specific subdirectories ("BlenderPro", "Lato", "Poppins")
     * located in potential resource paths (application resources, relative paths).
     * @return True if all expected font families were loaded successfully, false otherwise.
     */
    bool Initialize();

    /**
     * @brief Gets the singleton instance of the FontManager.
     * @return Pointer to the singleton FontManager instance.
     */
    static FontManager *GetInstance();

    /**
    * @brief Creates a QFont object with the specified logical name, size, and weight.
    *
    * Uses GetFontFamily to resolve the logical name and then construct a QFont object.
    * @param font_name The logical name of the font.
    * @param point_size The desired point size for the font. Defaults to 10.
    * @param weight The desired font weight (e.g., QFont::Normal, QFont::Bold). Defaults to QFont::Normal.
    * @return A QFont object configured with the specified parameters.
    */
    QFont GetFont(const QString &font_name, int point_size = 10, int weight = QFont::Normal);

    /**
     * @brief Retrieves the actual font family name associated with a logical font name.
     *
     * If the requested logical font name is found, its corresponding system family name is returned.
     * Otherwise, a warning is logged, and the default system font family is returned.
     * @param font_name The logical name of the font (e.g., "BlenderPro", "Lato").
     * @return The actual font family name (e.g., "Blender Pro Book") or the default font family name if not found.
     */
    QString GetFontFamily(const QString &font_name);

private:
    /**
     * @brief Private constructor to enforce the singleton pattern.
     * @param parent Optional parent QObject.
     */
    explicit FontManager(QObject *parent = nullptr);

    /**
     * @brief Private default destructor.
     */
    ~FontManager() override = default;

    /**
     * @brief Static pointer to the singleton instance.
     */
    static FontManager *instance_;

    /**
     * @brief Map storing the mapping between logical font names and actual font family names.
     * Key: Logical font name (e.g., "Poppins").
     * Value: Actual font family name loaded by QFontDatabase (e.g., "Poppins Regular").
     */
    QMap<QString, QString> font_families_;

    /**
     * @brief Loads a single font file into the application using QFontDatabase.
     *
     * Attempts to add the font file at the given path. If successful, it retrieves the
     * font family name and stores the mapping in font_families_.
     * @param font_name The logical name to associate with this font.
     * @param font_path The path to the font file (.ttf).
     * @return True if the font was loaded successfully and a family name was retrieved, false otherwise.
     */
    bool LoadFont(const QString &font_name, const QString &font_path);
};

#endif // FONT_MANAGER_H
