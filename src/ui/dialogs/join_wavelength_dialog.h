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
    double GetDigitalizationProgress() const { return digitalization_progress_; }
    void SetDigitalizationProgress(double progress);

    double GetCornerGlowProgress() const { return corner_glow_progress_; }
    void SetCornerGlowProgress(double progress);

    double GetScanlineOpacity() const { return scanline_opacity_; }
    void SetScanlineOpacity(double opacity);

    void paintEvent(QPaintEvent *event) override;

    QString GetFrequency() const;

    QString GetPassword() const {
        return password_edit_->text();
    }

    QTimer* GetRefreshTimer() const { return refresh_timer_; }
    void StartRefreshTimer() const {
        if (refresh_timer_) {
            refresh_timer_->start();
        }
    }
    void StopRefreshTimer() const {
        if (refresh_timer_) {
            refresh_timer_->stop();
        }
    }
    void SetRefreshTimerInterval(const int interval) const {
        if (refresh_timer_) {
            refresh_timer_->setInterval(interval);
        }
    }

private slots:
    void ValidateInput() const;

    void TryJoin();

    void OnAuthFailed();

    void OnConnectionError(const QString& error_message);

private:
    void InitRenderBuffers();

    bool animation_started_ = false;
    CyberLineEdit *frequency_edit_;
    QComboBox *frequency_unit_combo_;
    CyberLineEdit *password_edit_;
    QLabel *status_label_;
    CyberButton *join_button_;
    CyberButton *cancel_button_;
    QTimer *refresh_timer_;

    const int shadow_size_;
    double scanline_opacity_;
    double digitalization_progress_ = 0.0;
    double corner_glow_progress_ = 0.0;
    QPixmap scanline_buffer_;
    bool buffers_initialized_ = false;
    int last_scanline_y_ = -1;
    int previous_height_ = 0;
};

#endif // JOIN_WAVELENGTH_DIALOG_H