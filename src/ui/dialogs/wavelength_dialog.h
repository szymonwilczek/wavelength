#ifndef WAVELENGTH_DIALOG_H
#define WAVELENGTH_DIALOG_H

#include <QLabel>
#include <QApplication>

#include "../../session/wavelength_session_coordinator.h"
#include "../../ui/dialogs/animated_dialog.h"
#include "../../ui/buttons/cyber_button.h"
#include "../../ui/checkbox/cyber_checkbox.h"
#include "../../ui/input/cyber_line_edit.h"

class WavelengthDialog final : public AnimatedDialog {
    Q_OBJECT
     Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
     Q_PROPERTY(double digitalizationProgress READ digitalizationProgress WRITE setDigitalizationProgress)
     Q_PROPERTY(double cornerGlowProgress READ cornerGlowProgress WRITE setCornerGlowProgress)

public:
    explicit WavelengthDialog(QWidget *parent = nullptr);

    ~WavelengthDialog()  override;

    double GetDigitalizationProgress() const { return digitalization_progress_; }
    void SetDigitalizationProgress(double progress);

    double GetCornerGlowProgress() const { return corner_glow_progress_; }
    void SetCornerGlowProgress(double progress);

    double GetScanlineOpacity() const { return scanline_opacity_; }
    void SetScanlineOpacity(double opacity);

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

    void paintEvent(QPaintEvent *event) override;

    QString GetFrequency() const {
        return frequency_;
    }

    bool IsPasswordProtected() const {
        return password_protected_checkbox_->isChecked();
    }

    QString GetPassword() const {
        return password_edit_->text();
    }

private slots:
    void ValidateInputs() const;

    void StartFrequencySearch();

    void TryGenerate();

    void OnFrequencyFound();

    static QString FormatFrequencyText(double frequency);

private:
    static QString FindLowestAvailableFrequency();

    void InitRenderBuffers();

    bool animation_started_ = false;
    QLabel *frequency_label_;
    QLabel *loading_indicator_;
    CyberCheckBox *password_protected_checkbox_;
    CyberLineEdit *password_edit_;
    QLabel *status_label_;
    CyberButton *generate_button_;
    CyberButton *cancel_button_;
    QFutureWatcher<QString> *frequency_watcher_;
    QTimer *refresh_timer_;
    QString frequency_ = "130.0";
    bool frequency_found_ = false;
    const int shadow_size_;
    double scanline_opacity_;
    QPixmap scanline_buffer_;
    bool buffers_initialized_ = false;
    int last_scanline_y_ = -1;
    int previous_height_ = 0;
};

#endif // WAVELENGTH_DIALOG_H