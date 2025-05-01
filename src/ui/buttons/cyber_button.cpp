#include "cyber_button.h"

#include <QPainter>

CyberButton::CyberButton(const QString &text, QWidget *parent, const bool isPrimary): QPushButton(text, parent), glow_intensity_(0.5), is_primary_(isPrimary) {
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("background-color: transparent; border: none; font-family: Consolas; font-size: 9pt; font-weight: bold;");
}

void CyberButton::SetGlowIntensity(const double intensity) {
    glow_intensity_ = intensity;
    update();
}

void CyberButton::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Paleta kolorów zależna od typu przycisku
    QColor background_color, border_color, text_color, glow_color;

    if (is_primary_) {
        background_color = QColor(0, 40, 60);
        border_color = QColor(0, 200, 255);
        text_color = QColor(0, 220, 255);
        glow_color = QColor(0, 150, 255, 50);
    } else {
        background_color = QColor(40, 23, 41);
        border_color = QColor(207, 56, 110);
        text_color = QColor(230, 70, 120);
        glow_color = QColor(200, 50, 100, 50);
    }

    // Ścieżka przycisku ze ściętymi rogami
    QPainterPath path;
    int clip_size = 5; // rozmiar ścięcia

    path.moveTo(clip_size, 0);
    path.lineTo(width() - clip_size, 0);
    path.lineTo(width(), clip_size);
    path.lineTo(width(), height() - clip_size);
    path.lineTo(width() - clip_size, height());
    path.lineTo(clip_size, height());
    path.lineTo(0, height() - clip_size);
    path.lineTo(0, clip_size);
    path.closeSubpath();

    // Efekt poświaty
    if (glow_intensity_ > 0.2) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow_color);

        for (int i = 3; i > 0; i--) {
            double glow_size = i * 2.0 * glow_intensity_;
            QPainterPath glow_path;
            glow_path.addRoundedRect(rect().adjusted(-glow_size, -glow_size, glow_size, glow_size), 4, 4);
            painter.setOpacity(0.15 * glow_intensity_);
            painter.drawPath(glow_path);
        }
        painter.setOpacity(1.0);
    }

    // Tło przycisku
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(border_color, 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Ozdobne linie wewnętrzne
    painter.setPen(QPen(border_color.darker(150), 1, Qt::DotLine));
    painter.drawLine(5, 5, width() - 5, 5);
    painter.drawLine(5, height() - 5, width() - 5, height() - 5);

    // Znaczniki w rogach
    int marker_size = 3;
    painter.setPen(QPen(border_color, 1, Qt::SolidLine));

    // Lewy górny
    painter.drawLine(clip_size + 2, 3, clip_size + 2 + marker_size, 3);
    painter.drawLine(clip_size + 2, 3, clip_size + 2, 3 + marker_size);

    // Prawy górny
    painter.drawLine(width() - clip_size - 2 - marker_size, 3, width() - clip_size - 2, 3);
    painter.drawLine(width() - clip_size - 2, 3, width() - clip_size - 2, 3 + marker_size);

    // Prawy dolny
    painter.drawLine(width() - clip_size - 2 - marker_size, height() - 3, width() - clip_size - 2, height() - 3);
    painter.drawLine(width() - clip_size - 2, height() - 3, width() - clip_size - 2, height() - 3 - marker_size);

    // Lewy dolny
    painter.drawLine(clip_size + 2, height() - 3, clip_size + 2 + marker_size, height() - 3);
    painter.drawLine(clip_size + 2, height() - 3, clip_size + 2, height() - 3 - marker_size);

    // Tekst przycisku
    painter.setPen(QPen(text_color, 1));
    painter.setFont(font());

    // Efekt przesunięcia dla stanu wciśniętego
    if (isDown()) {
        painter.drawText(rect().adjusted(1, 1, 1, 1), Qt::AlignCenter, text());
    } else {
        painter.drawText(rect(), Qt::AlignCenter, text());
    }

    // Jeśli nieaktywny, dodajemy efekt przyciemnienia
    if (!isEnabled()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 120));
        painter.drawPath(path);

        // Linie "zakłóceń" dla efektu nieaktywności
        painter.setPen(QPen(QColor(50, 50, 50, 80), 1, Qt::DotLine));
        for (int y = 0; y < height(); y += 3) {
            painter.drawLine(0, y, width(), y);
        }
    }
}

void CyberButton::enterEvent(QEvent *event) {
    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(200);
    animation->setStartValue(glow_intensity_);
    animation->setEndValue(0.9);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    QPushButton::enterEvent(event);
}

void CyberButton::leaveEvent(QEvent *event) {
    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(200);
    animation->setStartValue(glow_intensity_);
    animation->setEndValue(0.5);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
    QPushButton::leaveEvent(event);
}

void CyberButton::mousePressEvent(QMouseEvent *event) {
    glow_intensity_ = 1.0;
    update();
    QPushButton::mousePressEvent(event);
}

void CyberButton::mouseReleaseEvent(QMouseEvent *event) {
    glow_intensity_ = 0.9;
    update();
    QPushButton::mouseReleaseEvent(event);
}
