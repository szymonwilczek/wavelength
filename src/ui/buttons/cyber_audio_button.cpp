#include "cyber_audio_button.h"

CyberAudioButton::CyberAudioButton(const QString &text, QWidget *parent): QPushButton(text, parent), m_glowIntensity(0.5) {
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("background-color: transparent; border: none;");

    // Timer dla subtelnych efektów
    m_pulseTimer = new QTimer(this);
    connect(m_pulseTimer, &QTimer::timeout, this, [this]() {
        const double phase = sin(QDateTime::currentMSecsSinceEpoch() * 0.002) * 0.1;
        setGlowIntensity(m_baseGlowIntensity + phase);
    });
    m_pulseTimer->start(50);

    m_baseGlowIntensity = 0.5;
}

CyberAudioButton::~CyberAudioButton() {
    if (m_pulseTimer) {
        m_pulseTimer->stop();
        m_pulseTimer->deleteLater();
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
    int clipSize = 4; // mniejszy rozmiar ścięcia dla mniejszego przycisku

    path.moveTo(clipSize, 0);
    path.lineTo(width() - clipSize, 0);
    path.lineTo(width(), clipSize);
    path.lineTo(width(), height() - clipSize);
    path.lineTo(width() - clipSize, height());
    path.lineTo(clipSize, height());
    path.lineTo(0, height() - clipSize);
    path.lineTo(0, clipSize);
    path.closeSubpath();

    // Efekt poświaty przy hover/pressed
    if (m_glowIntensity > 0.2) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(glowColor);

        for (int i = 3; i > 0; i--) {
            double glowSize = i * 1.5 * m_glowIntensity;
            QPainterPath glowPath;
            glowPath.addRoundedRect(rect().adjusted(-glowSize, -glowSize, glowSize, glowSize), 3, 3);
            painter.setOpacity(0.15 * m_glowIntensity);
            painter.drawPath(glowPath);
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
    int markerSize = 2;
    painter.setPen(QPen(borderColor, 1, Qt::SolidLine));

    // Lewy górny marker
    painter.drawLine(clipSize + 1, 2, clipSize + 1 + markerSize, 2);
    painter.drawLine(clipSize + 1, 2, clipSize + 1, 2 + markerSize);

    // Prawy górny marker
    painter.drawLine(width() - clipSize - 1 - markerSize, 2, width() - clipSize - 1, 2);
    painter.drawLine(width() - clipSize - 1, 2, width() - clipSize - 1, 2 + markerSize);

    // Prawy dolny marker
    painter.drawLine(width() - clipSize - 1 - markerSize, height() - 2, width() - clipSize - 1, height() - 2);
    painter.drawLine(width() - clipSize - 1, height() - 2, width() - clipSize - 1, height() - 2 - markerSize);

    // Lewy dolny marker
    painter.drawLine(clipSize + 1, height() - 2, clipSize + 1 + markerSize, height() - 2);
    painter.drawLine(clipSize + 1, height() - 2, clipSize + 1, height() - 2 - markerSize);

    // Tekst przycisku
    painter.setPen(QPen(textColor, 1));
    painter.setFont(QFont("Consolas", 9, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
