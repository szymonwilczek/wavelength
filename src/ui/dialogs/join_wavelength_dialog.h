#ifndef JOIN_WAVELENGTH_DIALOG_H
#define JOIN_WAVELENGTH_DIALOG_H

#include <QComboBox>

#include "wavelength_dialog.h"
#include "../../session/wavelength_session_coordinator.h"
#include "../../ui/dialogs/animated_dialog.h"

class JoinWavelengthDialog final : public AnimatedDialog {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
    Q_PROPERTY(double digitalizationProgress READ digitalizationProgress WRITE setDigitalizationProgress)
    Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)

public:
    explicit JoinWavelengthDialog(QWidget *parent = nullptr);

    ~JoinWavelengthDialog() override;

    // Akcesory do animacji
    double digitalizationProgress() const { return m_digitalizationProgress; }
    void setDigitalizationProgress(double progress);

    double cornerGlowProgress() const { return m_cornerGlowProgress; }
    void setCornerGlowProgress(double progress);


    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity);

    void paintEvent(QPaintEvent *event) override;

    QString getFrequency() const;

    QString getPassword() const {
        return passwordEdit->text();
    }

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
    void setRefreshTimerInterval(const int interval) const {
        if (m_refreshTimer) {
            m_refreshTimer->setInterval(interval);
        }
    }

private slots:
    void validateInput() const;

    void tryJoin();

    void onAuthFailed(QString frequency);

    void onConnectionError(const QString& errorMessage);

private:
    void initRenderBuffers();

    bool m_animationStarted = false;
    CyberLineEdit *frequencyEdit;
    QComboBox *frequencyUnitCombo;
    CyberLineEdit *passwordEdit;
    QLabel *statusLabel;
    CyberButton *joinButton;
    CyberButton *cancelButton;
    QTimer *m_refreshTimer;

    const int m_shadowSize;
    double m_scanlineOpacity;
    double m_digitalizationProgress = 0.0;
    double m_cornerGlowProgress = 0.0;
    QPixmap m_scanlineBuffer;
    bool m_buffersInitialized = false;
    int m_lastScanlineY = -1;
    int m_previousHeight = 0;
};

#endif // JOIN_WAVELENGTH_DIALOG_H