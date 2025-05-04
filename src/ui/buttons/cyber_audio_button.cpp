#include "cyber_audio_button.h"

#include <cmath>

CyberAudioButton::CyberAudioButton(const QString &text, QWidget *parent): QPushButton(text, parent), glow_intensity_(0.5) {
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("background-color: transparent; border: none;");

    // Timer dla subtelnych efektów
    pulse_timer_ = new QTimer(this);
    connect(pulse_timer_, &QTimer::timeout, this, [this]() {
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

    // Paleta kolorów - fioletowo-niebieska paleta dla audio
    QColor bgColor(30, 20, 60);       // ciemny tło
    QColor borderColor(140, 80, 240); // neonowy fioletowy brzeg
    QColor textColor(160, 100, 255);  // tekst fioletowy

    // Ścieżka przycisku - ścięte rogi dla cyberpunkowego stylu
    QPainterPath path;
    int clip_size = 4; // mniejszy rozmiar ścięcia dla mniejszego przycisku

    path.moveTo(clip_size, 0);
    path.lineTo(width() - clip_size, 0);
    path.lineTo(width(), clip_size);
    path.lineTo(width(), height() - clip_size);
    path.lineTo(width() - clip_size, height());
    path.lineTo(clip_size, height());
    path.lineTo(0, height() - clip_size);
    path.lineTo(0, clip_size);
    path.closeSubpath();

    // Tło przycisku
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(borderColor, 1.2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Tekst przycisku
    painter.setPen(QPen(textColor, 1));
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
