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

class AppearanceSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AppearanceSettingsWidget(QWidget *parent = nullptr);
    ~AppearanceSettingsWidget() override = default;

    void loadSettings();
    void saveSettings();

    private slots:
        // Istniejące sloty
        void chooseBackgroundColor();
    void chooseBlobColor();
    void chooseStreamColor();
    void updateRecentColorsUI();
    void selectRecentColor(const QColor& color);

    // NOWE sloty
    void chooseGridColor();
    void gridSpacingChanged(int value);
    void chooseTitleTextColor();
    void chooseTitleBorderColor();
    void chooseTitleGlowColor();

private:
    void setupUi();
    void updateColorPreview(QWidget* previewWidget, const QColor& color);

    WavelengthConfig *m_config;

    // Istniejące elementy UI i zmienne
    QWidget* m_bgColorPreview;
    QWidget* m_blobColorPreview;
    QWidget* m_streamColorPreview;
    QColor m_selectedBgColor;
    QColor m_selectedBlobColor;
    QColor m_selectedStreamColor;

    // NOWE elementy UI i zmienne
    QWidget* m_gridColorPreview;
    QSpinBox* m_gridSpacingSpinBox;
    QWidget* m_titleTextColorPreview;
    QWidget* m_titleBorderColorPreview;
    QWidget* m_titleGlowColorPreview;

    QColor m_selectedGridColor;
    int m_selectedGridSpacing;
    QColor m_selectedTitleTextColor;
    QColor m_selectedTitleBorderColor;
    QColor m_selectedTitleGlowColor;
    // -----------------------------

    QHBoxLayout* m_recentColorsLayout;
    QList<QPushButton*> m_recentColorButtons;
};

#endif // APPEARANCE_SETTINGS_WIDGET_H