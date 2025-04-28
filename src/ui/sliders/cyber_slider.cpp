//
// Created by szymo on 28.04.2025.
//

#include "cyber_slider.h"

CyberSlider::CyberSlider(Qt::Orientation orientation, QWidget *parent): QSlider(orientation, parent), m_glowIntensity(0.5) {
    setStyleSheet("background: transparent; border: none;");
}

void CyberSlider::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paleta kolorów cyberpunkowych
    QColor trackColor(0, 60, 80);            // ciemny niebieski
    QColor progressColor(0, 200, 255);       // neonowy niebieski
    QColor handleColor(0, 240, 255);         // jaśniejszy neon
    QColor glowColor(0, 220, 255, 80);       // poświata

    const int handleWidth = 14;
    const int handleHeight = 20;
    const int trackHeight = 4;

    // Rysowanie ścieżki
    QRect trackRect = rect().adjusted(5, (height() - trackHeight) / 2, -5, -(height() - trackHeight) / 2);

    // Tworzymy ścieżkę z zaokrągleniem
    QPainterPath trackPath;
    trackPath.addRoundedRect(trackRect, 2, 2);

    // Cieniowanie dla ścieżki
    painter.setPen(Qt::NoPen);
    painter.setBrush(trackColor);
    painter.drawPath(trackPath);

    // Oblicz pozycję wskaźnika
    int handlePos = QStyle::sliderPositionFromValue(minimum(), maximum(), value(), width() - handleWidth);

    // Rysowanie wypełnionej części
    if (value() > minimum()) {
        QRect progressRect = trackRect;
        progressRect.setWidth(handlePos + handleWidth/2);

        QPainterPath progressPath;
        progressPath.addRoundedRect(progressRect, 2, 2);

        painter.setBrush(progressColor);
        painter.drawPath(progressPath);

        // Dodajemy linie skanujące w wypełnionej części
        painter.setOpacity(0.4);
        painter.setPen(QPen(QColor(255, 255, 255, 40), 1));

        for (int i = 0; i < progressRect.width(); i += 15) {
            painter.drawLine(i, progressRect.top(), i, progressRect.bottom());
        }
        painter.setOpacity(1.0);
    }

    // Rysowanie uchwytu z efektem świecenia
    QRect handleRect(handlePos, (height() - handleHeight) / 2, handleWidth, handleHeight);

    // Poświata neonu
    if (m_glowIntensity > 0.2) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(glowColor);

        for (int i = 4; i > 0; i--) {
            double glowSize = i * 2.5 * m_glowIntensity;
            QRect glowRect = handleRect.adjusted(-glowSize, -glowSize, glowSize, glowSize);
            painter.setOpacity(0.15 * m_glowIntensity);
            painter.drawRoundedRect(glowRect, 5, 5);
        }

        painter.setOpacity(1.0);
    }

    // Rysowanie uchwytu
    QPainterPath handlePath;
    handlePath.addRoundedRect(handleRect, 3, 3);

    painter.setPen(QPen(handleColor, 1));
    painter.setBrush(QColor(0, 50, 70));
    painter.drawPath(handlePath);

    // Dodajemy wewnętrzne linie dla efektu technologicznego
    painter.setPen(QPen(handleColor.lighter(), 1));
    painter.drawLine(handleRect.left() + 4, handleRect.top() + 4,
                     handleRect.right() - 4, handleRect.top() + 4);
    painter.drawLine(handleRect.left() + 4, handleRect.bottom() - 4,
                     handleRect.right() - 4, handleRect.bottom() - 4);
}

void CyberSlider::enterEvent(QEvent *event) {
    // Animowana poświata przy najechaniu
    QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
    anim->setDuration(300);
    anim->setStartValue(m_glowIntensity);
    anim->setEndValue(0.9);
    anim->start(QPropertyAnimation::DeleteWhenStopped);

    QSlider::enterEvent(event);
}

void CyberSlider::leaveEvent(QEvent *event) {
    // Wygaszenie poświaty przy opuszczeniu
    QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
    anim->setDuration(300);
    anim->setStartValue(m_glowIntensity);
    anim->setEndValue(0.5);
    anim->start(QPropertyAnimation::DeleteWhenStopped);

    QSlider::leaveEvent(event);
}
