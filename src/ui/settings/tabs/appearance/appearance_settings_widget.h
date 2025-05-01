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

class AppearanceSettingsWidget final : public QWidget {
    Q_OBJECT

public:
    explicit AppearanceSettingsWidget(QWidget *parent = nullptr);
    ~AppearanceSettingsWidget() override = default;

    void LoadSettings();
    void SaveSettings() const;

    private slots:
        void ChooseBackgroundColor();
        void ChooseBlobColor();
        void ChooseStreamColor();
        void UpdateRecentColorsUI();
        void SelectRecentColor(const QColor& color) const;
        void ChooseGridColor();
        void GridSpacingChanged(int value);
        void ChooseTitleTextColor();
        void ChooseTitleBorderColor();
        void ChooseTitleGlowColor();

private:
    void SetupUi();

    static void UpdateColorPreview(QWidget* preview_widget, const QColor& color);

    WavelengthConfig *m_config;

    // Istniejące elementy UI i zmienne
    QWidget* bg_color_preview_;
    QWidget* blob_color_preview_;
    QWidget* stream_color_preview_;
    QColor selected_background_color_;
    QColor selected_blob_color_;
    QColor selected_stream_color_;

    // NOWE elementy UI i zmienne
    QWidget* grid_color_preview_;
    QSpinBox* grid_spacing_spin_box_;
    QWidget* title_text_color_preview_;
    QWidget* title_border_color_preview_;
    QWidget* title_glow_color_preview_;

    QColor selected_grid_color_;
    int selected_grid_spacing_;
    QColor selected_title_text_color_;
    QColor selected_title_border_color_;
    QColor selected_title_glow_color_;
    // -----------------------------

    QHBoxLayout* recent_colors_layout_;
    QList<QPushButton*> recent_color_buttons_;
};

#endif // APPEARANCE_SETTINGS_WIDGET_H