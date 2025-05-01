#ifndef VIDEO_PLAYER_OVERLAY_H
#define VIDEO_PLAYER_OVERLAY_H

#include <QDialog>
#include <QLabel>
#include <QCloseEvent>
#include <QRandomGenerator>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include "../../../../ui/buttons/cyber_push_button.h"
#include "../../../../ui/sliders/cyber_slider.h"
#include "../decoder/video_decoder.h"

class VideoPlayerOverlay final : public QDialog {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
    Q_PROPERTY(double gridOpacity READ gridOpacity WRITE setGridOpacity)

public:
    VideoPlayerOverlay(const QByteArray& video_data, const QString& mime_type, QWidget* parent = nullptr);

    ~VideoPlayerOverlay() override;

    void ReleaseResources();

    // Akcesory dla właściwości animacji
    double GetScanlineOpacity() const { return scanline_opacity_; }
    void SetScanlineOpacity(const double opacity) {
        scanline_opacity_ = opacity;
        update();
    }

    double GetGridOpacity() const { return grid_opacity_; }
    void SetGridOpacity(const double opacity) {
        grid_opacity_ = opacity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

    void closeEvent(QCloseEvent* event) override;

private slots:
    void InitializePlayer();

    void TogglePlayback();

    void OnSliderPressed();

    void OnSliderReleased();

    void UpdateTimeLabel(int position) const;

    void UpdateSliderPosition(double position) const;

    void SeekVideo(int position) const;

    void UpdateFrame(const QImage& frame);

    void UpdateUI();

    void HandleError(const QString& message) const;

    void HandleVideoInfo(int width, int height, double fps, double duration);

    void AdjustVolume(int volume) const;

    void ToggleMute();

    void UpdateVolumeIcon(float volume) const;

    void TriggerGlitch();

private:
    QLabel* video_label_;
    CyberPushButton* play_button_;
    CyberSlider* progress_slider_;
    QLabel* time_label_;
    CyberPushButton* volume_button_;
    CyberSlider* volume_slider_;
    QLabel* status_label_;
    QLabel* codec_label_;
    QLabel* resolution_label_;
    QLabel* bitrate_label_;
    QLabel* fps_label_;

    std::shared_ptr<VideoDecoder> decoder_;
    QTimer* update_timer_;
    QTimer* glitch_timer_;

    QByteArray video_data_;
    QString mime_type_;

    int video_width_ = 0;
    int video_height_ = 0;
    double video_duration_ = 0;
    bool slider_dragging_ = false;
    int last_volume_ = 100;
    bool playback_finished_ = false;
    bool was_playing_ = false;
    QImage thumbnail_frame_;

    double scanline_opacity_;
    double grid_opacity_;
    bool show_hud_ = false;
    int frame_counter_ = 0;
    bool playback_started_ = false;
    double current_glitch_intensity_ = 0.0;
    double video_fps_ = 60.0;
};

#endif //VIDEO_PLAYER_OVERLAY_H