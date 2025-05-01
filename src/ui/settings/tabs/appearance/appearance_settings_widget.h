#ifndef APPEARANCE_SETTINGS_WIDGET_H
#define APPEARANCE_SETTINGS_WIDGET_H

#include <QWidget>
#include <QColor>
#include <QStringList>

class WavelengthConfig;
class QLabel;
class QPushButton;
class QHBoxLayout;
class QGridLayout;
class QColorDialog;
class QSpinBox;
class ClickableColorPreview;

/**
 * @brief A widget for configuring the application's appearance settings.
 *
 * This widget provides UI elements (color previews, spin boxes) to allow the user
 * to customize various visual aspects of the application, such as background color,
 * blob color, stream color, grid appearance, and title bar styling.
 * It interacts with the WavelengthConfig singleton to load and save these settings.
 * Changes made via the UI are immediately applied to the WavelengthConfig, triggering
 * updates in other parts of the application that listen to config changes.
 */
class AppearanceSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructs the AppearanceSettingsWidget.
     * Initializes the UI, loads current settings from WavelengthConfig, and connects
     * signals for UI interaction and config updates.
     * @param parent Optional parent widget.
     */
    explicit AppearanceSettingsWidget(QWidget *parent = nullptr);

    /**
     * @brief Default destructor.
     */
    ~AppearanceSettingsWidget() override = default;

    /**
     * @brief Loads the current appearance settings from WavelengthConfig and updates the UI elements.
     */
    void LoadSettings();

    /**
     * @brief Saves the currently selected settings back to WavelengthConfig.
     * Note: In the current implementation, settings are saved immediately upon change,
     * so this method primarily ensures consistency if called externally.
     */
    void SaveSettings() const;

private slots:
    /**
     * @brief Opens a QColorDialog to allow the user to select the background color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseBackgroundColor();

    /**
     * @brief Opens a QColorDialog to allow the user to select the blob color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseBlobColor();

    /**
     * @brief Opens a QColorDialog to allow the user to select the stream color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseStreamColor();

    /**
     * @brief Updates the UI section displaying recently used colors.
     * Clears the existing recent color buttons and recreates them based on the
     * current list from WavelengthConfig. Triggered by WavelengthConfig::recentColorsChanged.
     */
    void UpdateRecentColorsUI();

    /**
     * @brief Slot triggered when a recent color button is clicked.
     * Adds the selected color back to the top of the recent colors list in WavelengthConfig.
     * @param color The color that was clicked.
     */
    void SelectRecentColor(const QColor& color) const;

    /**
     * @brief Opens a QColorDialog to allow the user to select the grid color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseGridColor();

    /**
     * @brief Slot triggered when the grid spacing spin box value changes.
     * Immediately saves the new spacing value to WavelengthConfig.
     * @param value The new grid spacing value in pixels.
     */
    void GridSpacingChanged(int value);

    /**
     * @brief Opens a QColorDialog to allow the user to select the title text color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseTitleTextColor();

    /**
     * @brief Opens a QColorDialog to allow the user to select the title border color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseTitleBorderColor();

    /**
     * @brief Opens a QColorDialog to allow the user to select the title glow color.
     * Updates the local preview and immediately saves the selected color to WavelengthConfig.
     */
    void ChooseTitleGlowColor();

private:
    /**
     * @brief Creates and arranges all the UI elements (labels, previews, spin boxes, layouts) for the widget.
     */
    void SetupUi();

    /**
     * @brief Static utility function to update the background color of a ClickableColorPreview widget.
     * @param preview_widget Pointer to the QWidget (expected to be a ClickableColorPreview).
     * @param color The new color to set.
     */
    static void UpdateColorPreview(QWidget* preview_widget, const QColor& color);

    /** @brief Pointer to the WavelengthConfig singleton instance. */
    WavelengthConfig *m_config;

    // --- UI Elements ---
    /** @brief Clickable preview widget for the background color. */
    QWidget* bg_color_preview_;
    /** @brief Clickable preview widget for the blob color. */
    QWidget* blob_color_preview_;
    /** @brief Clickable preview widget for the stream color. */
    QWidget* stream_color_preview_;
    /** @brief Clickable preview widget for the grid color. */
    QWidget* grid_color_preview_;
    /** @brief Spin box for adjusting the grid spacing. */
    QSpinBox* grid_spacing_spin_box_;
    /** @brief Clickable preview widget for the title text color. */
    QWidget* title_text_color_preview_;
    /** @brief Clickable preview widget for the title border color. */
    QWidget* title_border_color_preview_;
    /** @brief Clickable preview widget for the title glow color. */
    QWidget* title_glow_color_preview_;
    /** @brief Horizontal layout containing the recent color preview buttons. */
    QHBoxLayout* recent_colors_layout_;
    /** @brief List storing pointers to the dynamically created recent color buttons. */
    QList<QPushButton*> recent_color_buttons_;
    // --- End UI Elements ---

    // --- Selected Values (mirror config for potential SaveSettings logic) ---
    /** @brief Currently selected background color. */
    QColor selected_background_color_;
    /** @brief Currently selected blob color. */
    QColor selected_blob_color_;
    /** @brief Currently selected stream color. */
    QColor selected_stream_color_;
    /** @brief Currently selected grid color. */
    QColor selected_grid_color_;
    /** @brief Currently selected grid spacing value. */
    int selected_grid_spacing_;
    /** @brief Currently selected title text color. */
    QColor selected_title_text_color_;
    /** @brief Currently selected title border color. */
    QColor selected_title_border_color_;
    /** @brief Currently selected title glow color. */
    QColor selected_title_glow_color_;
    // --- End Selected Values ---
};

#endif // APPEARANCE_SETTINGS_WIDGET_H