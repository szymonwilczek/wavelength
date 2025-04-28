//
// Created by szymo on 28.04.2025.
//

#ifndef CYBER_AUDIO_SLIDER_H
#define CYBER_AUDIO_SLIDER_H
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QSlider>
#include <QStyle>


class CyberAudioSlider : public QSlider {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberAudioSlider(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QSlider(orientation, parent), m_glowIntensity(0.5) {
        setStyleSheet("background: transparent; border: none;");
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Paleta kolorów cyberpunkowych - odcień fioletowo-niebieski dla audio
        QColor trackColor(40, 30, 80);            // ciemny fiolet
        QColor progressColor(140, 70, 240);       // neonowy fiolet
        QColor handleColor(160, 100, 255);        // jaśniejszy neon
        QColor glowColor(150, 80, 255, 80);       // poświata

        const int handleWidth = 12;
        const int handleHeight = 16;
        const int trackHeight = 3;

        // Rysowanie ścieżki
        QRect trackRect = rect().adjusted(5, (height() - trackHeight) / 2, -5, -(height() - trackHeight) / 2);

        // Ścieżka z zaokrągleniem
        QPainterPath trackPath;
        trackPath.addRoundedRect(trackRect, 1, 1);

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
            progressPath.addRoundedRect(progressRect, 1, 1);

            painter.setBrush(progressColor);
            painter.drawPath(progressPath);

            // Kreski w wypełnionej części
            painter.setOpacity(0.4);
            painter.setPen(QPen(QColor(255, 255, 255, 40), 1));

            for (int i = 0; i < progressRect.width(); i += 10) {
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

            for (int i = 3; i > 0; i--) {
                double glowSize = i * 2.0 * m_glowIntensity;
                QRect glowRect = handleRect.adjusted(-glowSize, -glowSize, glowSize, glowSize);
                painter.setOpacity(0.15 * m_glowIntensity);
                painter.drawRoundedRect(glowRect, 4, 4);
            }

            painter.setOpacity(1.0);
        }

        // Rysowanie uchwytu
        QPainterPath handlePath;
        handlePath.addRoundedRect(handleRect, 2, 2);

        painter.setPen(QPen(handleColor, 1));
        painter.setBrush(QColor(30, 20, 60));
        painter.drawPath(handlePath);

        // Dodajemy wewnętrzne linie dla efektu technologicznego
        painter.setPen(QPen(handleColor.lighter(), 1));
        painter.drawLine(handleRect.left() + 3, handleRect.top() + 3,
                        handleRect.right() - 3, handleRect.top() + 3);
        painter.drawLine(handleRect.left() + 3, handleRect.bottom() - 3,
                        handleRect.right() - 3, handleRect.bottom() - 3);
    }

    void enterEvent(QEvent* event) override {
        // Animowana poświata przy najechaniu
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(300);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.9);
        anim->start(QPropertyAnimation::DeleteWhenStopped);

        QSlider::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        // Wygaszenie poświaty przy opuszczeniu
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(300);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);

        QSlider::leaveEvent(event);
    }

private:
    double m_glowIntensity;
};



#endif //CYBER_AUDIO_SLIDER_H
