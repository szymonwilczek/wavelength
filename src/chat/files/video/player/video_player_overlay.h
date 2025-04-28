#ifndef VIDEO_PLAYER_OVERLAY_H
#define VIDEO_PLAYER_OVERLAY_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QToolButton>
#include <QTimer>
#include <QPainter>
#include <QCloseEvent>
#include <QPainterPath>
#include <QDateTime>
#include <QRandomGenerator>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStyle>
#include <QOperatingSystemVersion>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

#include "../../../../ui/buttons/cyber_push_button.h"
#include "../../../../ui/sliders/cyber_slider.h"
#include "../decoder/video_decoder.h"

class VideoPlayerOverlay : public QDialog {
    Q_OBJECT
    Q_PROPERTY(double scanlineOpacity READ scanlineOpacity WRITE setScanlineOpacity)
    Q_PROPERTY(double gridOpacity READ gridOpacity WRITE setGridOpacity)

public:
    VideoPlayerOverlay(const QByteArray& videoData, const QString& mimeType, QWidget* parent = nullptr);

    ~VideoPlayerOverlay() override;

    void releaseResources();

    // Akcesory dla właściwości animacji
    double scanlineOpacity() const { return m_scanlineOpacity; }
    void setScanlineOpacity(double opacity) {
        m_scanlineOpacity = opacity;
        update();
    }

    double gridOpacity() const { return m_gridOpacity; }
    void setGridOpacity(double opacity) {
        m_gridOpacity = opacity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;

    void closeEvent(QCloseEvent* event) override;

private slots:
    void initializePlayer();

    void togglePlayback();

    void onSliderPressed();

    void onSliderReleased();

    void updateTimeLabel(int position);

    void updateSliderPosition(double position);

    void seekVideo(int position) const;

    void updateFrame(const QImage& frame);

    void updateUI();

    void handleError(const QString& message) const;

    void handleVideoInfo(int width, int height, double fps, double duration);

    void adjustVolume(int volume);

    void toggleMute();

    void updateVolumeIcon(float volume) const;

    void triggerGlitch();

private:
    QLabel* m_videoLabel;
    CyberPushButton* m_playButton;
    CyberSlider* m_progressSlider;
    QLabel* m_timeLabel;
    CyberPushButton* m_volumeButton;
    CyberSlider* m_volumeSlider;
    QLabel* m_statusLabel;
    QLabel* m_codecLabel;
    QLabel* m_resolutionLabel;
    QLabel* m_bitrateLabel;
    QLabel* m_fpsLabel;

    std::shared_ptr<VideoDecoder> m_decoder;
    QTimer* m_updateTimer;
    QTimer* m_glitchTimer;

    QByteArray m_videoData;
    QString m_mimeType;

    int m_videoWidth = 0;
    int m_videoHeight = 0;
    double m_videoDuration = 0;
    bool m_sliderDragging = false;
    int m_lastVolume = 100;
    bool m_playbackFinished = false;
    bool m_wasPlaying = false;
    QImage m_thumbnailFrame;

    // Właściwości animacji
    double m_scanlineOpacity;
    double m_gridOpacity;
    bool m_showHUD = false;
    int m_frameCounter = 0;
    bool m_playbackStarted = false;
    double m_currentGlitchIntensity = 0.0;
    double m_videoFps = 60.0;
};

#endif //VIDEO_PLAYER_OVERLAY_H