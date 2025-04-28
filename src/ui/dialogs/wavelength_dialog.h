#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QApplication>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QNetworkReply>

#include "../../session/wavelength_session_coordinator.h"
#include "../../ui/dialogs/animated_dialog.h"
#include "../../ui/buttons/cyber_button.h"
#include "../../ui/checkbox/cyber_checkbox.h"
#include "../../ui/input/cyber_line_edit.h"

class WavelengthDialog : public AnimatedDialog {
    Q_OBJECT
     Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
     Q_PROPERTY(double digitalizationProgress READ digitalizationProgress WRITE setDigitalizationProgress)
     Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)

public:
    explicit WavelengthDialog(QWidget *parent = nullptr);

    ~WavelengthDialog()  override;

    double digitalizationProgress() const { return m_digitalizationProgress; }
    void setDigitalizationProgress(double progress);

    double cornerGlowProgress() const { return m_cornerGlowProgress; }
    void setCornerGlowProgress(double progress);


    // Akcesory dla animacji scanlines
    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity);

    QTimer* getRefreshTimer() const { return m_refreshTimer; }
    void startRefreshTimer() const {
        if (m_refreshTimer) {
            m_refreshTimer->start();
        }
    }
    void stopRefreshTimer() const {
        if (m_refreshTimer) {
            m_refreshTimer->stop();
        }
    }
    void setRefreshTimerInterval(int interval) const {
        if (m_refreshTimer) {
            m_refreshTimer->setInterval(interval);
        }
    }

    void paintEvent(QPaintEvent *event) override;

    QString getFrequency() const {
        return m_frequency;
    }

    bool isPasswordProtected() const {
        return passwordProtectedCheckbox->isChecked();
    }

    QString getPassword() const {
        return passwordEdit->text();
    }

private slots:
    void validateInputs() const;

    void startFrequencySearch();

    void tryGenerate();

    void onFrequencyFound();

    // Nowa pomocnicza metoda do formatowania tekstu częstotliwości
    static QString formatFrequencyText(double frequency);

private:
    static QString findLowestAvailableFrequency();

    void initRenderBuffers();

private:
    bool m_animationStarted = false;
    QLabel *frequencyLabel;
    QLabel *loadingIndicator;
    CyberCheckBox *passwordProtectedCheckbox;
    CyberLineEdit *passwordEdit;
    QLabel *statusLabel;
    CyberButton *generateButton;
    CyberButton *cancelButton;
    QFutureWatcher<QString> *frequencyWatcher;
    QTimer *m_refreshTimer;
    QString m_frequency = "130.0"; // Domyślna wartość
    bool m_frequencyFound = false; // Flaga oznaczająca znalezienie częstotliwości
    const int m_shadowSize; // Rozmiar cienia
    double m_scanlineOpacity; // Przezroczystość linii skanowania
    QPixmap m_scanlineBuffer;
    bool m_buffersInitialized = false;
    int m_lastScanlineY = -1;
    int m_previousHeight = 0;
};

#endif // WAVELENGTH_DIALOG_H