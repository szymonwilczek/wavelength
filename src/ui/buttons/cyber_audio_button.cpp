#include "cyber_audio_button.h"

#include <cmath>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

CyberAudioButton::CyberAudioButton(const QString &text, QWidget *parent): QPushButton(text, parent),
                                                                          glow_intensity_(0.5) {
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("background-color: transparent; border: none;");

    pulse_timer_ = new QTimer(this);
    connect(pulse_timer_, &QTimer::timeout, this, [this] {
        const double phase = sin(QDateTime::currentMSecsSinceEpoch() * 0.002) * 0.1;
        SetGlowIntensity(base_glow_intensity_ + phase);
    });
    pulse_timer_->start(50);

    base_glow_intensity_ = 0.5;
}

CyberAudioButton::~CyberAudioButton() {
    if (pulse_timer_) {
        pulse_timer_->stop();
        pulse_timer_->deleteLater();
    }
}

void CyberAudioButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bgColor(30, 20, 60);
    QColor borderColor(140, 80, 240);
    QColor textColor(160, 100, 255);

    QPainterPath path;
    int clip_size = 4;

    path.moveTo(clip_size, 0);
    path.lineTo(width() - clip_size, 0);
    path.lineTo(width(), clip_size);
    path.lineTo(width(), height() - clip_size);
    path.lineTo(width() - clip_size, height());
    path.lineTo(clip_size, height());
    path.lineTo(0, height() - clip_size);
    path.lineTo(0, clip_size);
    path.closeSubpath();

    // button background
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    // button border
    painter.setPen(QPen(borderColor, 1.2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // button text
    painter.setPen(QPen(textColor, 1));
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
