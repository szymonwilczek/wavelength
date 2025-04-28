#include "cyber_push_button.h"

CyberPushButton::CyberPushButton(const QString &text, QWidget *parent): QPushButton(text, parent), m_glowIntensity(0.5) {
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

void CyberPushButton::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paleta kolorów
    QColor bgColor(0, 40, 60);        // ciemny tył
    QColor borderColor(0, 200, 255);  // neonowy niebieski brzeg
    QColor textColor(0, 220, 255);    // neonowy tekst
    QColor glowColor(0, 150, 255, 50); // poświata

    // Tworzymy ścieżkę przycisku - ścięte rogi dla cyberpunkowego stylu
    QPainterPath path;
    int clipSize = 5; // rozmiar ścięcia

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
            double glowSize = i * 2.0 * m_glowIntensity;
            QPainterPath glowPath;
            glowPath.addRoundedRect(rect().adjusted(-glowSize, -glowSize, glowSize, glowSize), 4, 4);
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
    painter.setPen(QPen(borderColor, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Ozdobne linie wewnętrzne
    painter.setPen(QPen(borderColor.darker(150), 1, Qt::DotLine));
    painter.drawLine(5, 5, width() - 5, 5);
    painter.drawLine(5, height() - 5, width() - 5, height() - 5);

    // Znaczniki w rogach
    int markerSize = 3;
    painter.setPen(QPen(borderColor, 1, Qt::SolidLine));

    // Lewy górny marker
    painter.drawLine(clipSize + 2, 3, clipSize + 2 + markerSize, 3);
    painter.drawLine(clipSize + 2, 3, clipSize + 2, 3 + markerSize);

    // Prawy górny marker
    painter.drawLine(width() - clipSize - 2 - markerSize, 3, width() - clipSize - 2, 3);
    painter.drawLine(width() - clipSize - 2, 3, width() - clipSize - 2, 3 + markerSize);

    // Prawy dolny marker
    painter.drawLine(width() - clipSize - 2 - markerSize, height() - 3, width() - clipSize - 2, height() - 3);
    painter.drawLine(width() - clipSize - 2, height() - 3, width() - clipSize - 2, height() - 3 - markerSize);

    // Lewy dolny marker
    painter.drawLine(clipSize + 2, height() - 3, clipSize + 2 + markerSize, height() - 3);
    painter.drawLine(clipSize + 2, height() - 3, clipSize + 2, height() - 3 - markerSize);

    // Tekst przycisku
    painter.setPen(QPen(textColor, 1));
    painter.setFont(QFont("Consolas", 10, QFont::Bold));
    painter.drawText(rect(), Qt::AlignCenter, text());
}
