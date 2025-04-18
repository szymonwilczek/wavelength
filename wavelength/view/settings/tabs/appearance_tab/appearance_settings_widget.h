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

class AppearanceSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit AppearanceSettingsWidget(QWidget *parent = nullptr);
    ~AppearanceSettingsWidget() override = default; // Domyślny destruktor wystarczy

    void loadSettings(); // Metoda do załadowania ustawień z WavelengthConfig
    void saveSettings(); // Metoda do zapisania ustawień do WavelengthConfig

    private slots:
        void chooseBackgroundColor();
    void chooseBlobColor();
    void chooseMessageColor();
    void chooseStreamColor();
    void updateRecentColorsUI();
    void selectRecentColor(const QColor& color);

private:
    void setupUi();
    void updateColorPreview(QWidget* previewWidget, const QColor& color);

    WavelengthConfig *m_config; // Wskaźnik do instancji konfiguracji

    // Elementy UI
    QWidget* m_bgColorPreview;
    QWidget* m_blobColorPreview;
    QWidget* m_messageColorPreview;
    QWidget* m_streamColorPreview;

    // Tymczasowo przechowywane wybrane kolory
    QColor m_selectedBgColor;
    QColor m_selectedBlobColor;
    QColor m_selectedMessageColor;
    QColor m_selectedStreamColor;

    // Kontener i przyciski dla ostatnich kolorów
    QHBoxLayout* m_recentColorsLayout;
    QList<QPushButton*> m_recentColorButtons;
};

#endif // APPEARANCE_SETTINGS_WIDGET_H