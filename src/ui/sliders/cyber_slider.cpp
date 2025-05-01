#include "cyber_slider.h"

#include <QPropertyAnimation>
#include <QStyle>

CyberSlider::CyberSlider(const Qt::Orientation orientation, QWidget *parent): QSlider(orientation, parent), glow_intensity_(0.5) {
    setStyleSheet("background: transparent; border: none;");
}

void CyberSlider::SetGlowIntensity(const double intensity) {
    glow_intensity_ = intensity;
    update();
}

void CyberSlider::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paleta kolorów cyberpunkowych
    QColor track_color(0, 60, 80);            // ciemny niebieski
    QColor progress_color(0, 200, 255);       // neonowy niebieski
    QColor handle_color(0, 240, 255);         // jaśniejszy neon
    QColor glow_color(0, 220, 255, 80);       // poświata

    constexpr int handle_width = 14;
    constexpr int handle_height = 20;
    constexpr int track_height = 4;

    // Rysowanie ścieżki
    QRect track_rect = rect().adjusted(5, (height() - track_height) / 2, -5, -(height() - track_height) / 2);

    // Tworzymy ścieżkę z zaokrągleniem
    QPainterPath track_path;
    track_path.addRoundedRect(track_rect, 2, 2);

    // Cieniowanie dla ścieżki
    painter.setPen(Qt::NoPen);
    painter.setBrush(track_color);
    painter.drawPath(track_path);

    // Oblicz pozycję wskaźnika
    int handle_position = QStyle::sliderPositionFromValue(minimum(), maximum(), value(), width() - handle_width);

    // Rysowanie wypełnionej części
    if (value() > minimum()) {
        QRect progress_rect = track_rect;
        progress_rect.setWidth(handle_position + handle_width/2);

        QPainterPath progress_path;
        progress_path.addRoundedRect(progress_rect, 2, 2);

        painter.setBrush(progress_color);
        painter.drawPath(progress_path);

        // Dodajemy linie skanujące w wypełnionej części
        painter.setOpacity(0.4);
        painter.setPen(QPen(QColor(255, 255, 255, 40), 1));

        for (int i = 0; i < progress_rect.width(); i += 15) {
            painter.drawLine(i, progress_rect.top(), i, progress_rect.bottom());
        }
        painter.setOpacity(1.0);
    }

    // Rysowanie uchwytu z efektem świecenia
    QRect handle_rect(handle_position, (height() - handle_height) / 2, handle_width, handle_height);

    // Poświata neonu
    if (glow_intensity_ > 0.2) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow_color);

        for (int i = 4; i > 0; i--) {
            double glow_size = i * 2.5 * glow_intensity_;
            QRect glow_rect = handle_rect.adjusted(-glow_size, -glow_size, glow_size, glow_size);
            painter.setOpacity(0.15 * glow_intensity_);
            painter.drawRoundedRect(glow_rect, 5, 5);
        }

        painter.setOpacity(1.0);
    }

    // Rysowanie uchwytu
    QPainterPath handle_path;
    handle_path.addRoundedRect(handle_rect, 3, 3);

    painter.setPen(QPen(handle_color, 1));
    painter.setBrush(QColor(0, 50, 70));
    painter.drawPath(handle_path);

    // Dodajemy wewnętrzne linie dla efektu technologicznego
    painter.setPen(QPen(handle_color.lighter(), 1));
    painter.drawLine(handle_rect.left() + 4, handle_rect.top() + 4,
                     handle_rect.right() - 4, handle_rect.top() + 4);
    painter.drawLine(handle_rect.left() + 4, handle_rect.bottom() - 4,
                     handle_rect.right() - 4, handle_rect.bottom() - 4);
}

void CyberSlider::enterEvent(QEvent *event) {
    // Animowana poświata przy najechaniu
    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(300);
    animation->setStartValue(glow_intensity_);
    animation->setEndValue(0.9);
    animation->start(QPropertyAnimation::DeleteWhenStopped);

    QSlider::enterEvent(event);
}

void CyberSlider::leaveEvent(QEvent *event) {
    // Wygaszenie poświaty przy opuszczeniu
    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(300);
    animation->setStartValue(glow_intensity_);
    animation->setEndValue(0.5);
    animation->start(QPropertyAnimation::DeleteWhenStopped);

    QSlider::leaveEvent(event);
}
