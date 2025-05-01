#include "cyber_audio_button.h"

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
    QColor glowColor(150, 80, 255, 50); // poświata

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

    // Efekt poświaty przy hover/pressed
    if (glow_intensity_ > 0.2) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(glowColor);

        for (int i = 3; i > 0; i--) {
            double glow_size = i * 1.5 * glow_intensity_;
            QPainterPath glow_path;
            glow_path.addRoundedRect(rect().adjusted(-glow_size, -glow_size, glow_size, glow_size), 3, 3);
            painter.setOpacity(0.15 * glow_intensity_);
            painter.drawPath(glow_path);
        }

        painter.setOpacity(1.0);
    }

    // Tło przycisku
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(borderColor, 1.2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Ozdobne linie wewnętrzne
    painter.setPen(QPen(borderColor.darker(150), 1, Qt::DotLine));
    painter.drawLine(4, 4, width() - 4, 4);
    painter.drawLine(4, height() - 4, width() - 4, height() - 4);

    // Znaczniki w rogach (mniejsze dla audio)
    int marker_size = 2;
    painter.setPen(QPen(borderColor, 1, Qt::SolidLine));

    // Lewy górny marker
    painter.drawLine(clip_size + 1, 2, clip_size + 1 + marker_size, 2);
    painter.drawLine(clip_size + 1, 2, clip_size + 1, 2 + marker_size);

    // Prawy górny marker
    painter.drawLine(width() - clip_size - 1 - marker_size, 2, width() - clip_size - 1, 2);
    painter.drawLine(width() - clip_size - 1, 2, width() - clip_size - 1, 2 + marker_size);

    // Prawy dolny marker
    painter.drawLine(width() - clip_size - 1 - marker_size, height() - 2, width() - clip_size - 1, height() - 2);
    painter.drawLine(width() - clip_size - 1, height() - 2, width() - clip_size - 1, height() - 2 - marker_size);

    // Lewy dolny marker
    painter.drawLine(clip_size + 1, height() - 2, clip_size + 1 + marker_size, height() - 2);
    painter.drawLine(clip_size + 1, height() - 2, clip_size + 1, height() - 2 - marker_size);

    // Tekst przycisku
    painter.setPen(QPen(textColor, 1));
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
