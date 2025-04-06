//
// Created by szymo on 6.04.2025.
//

#ifndef CYBER_BUTTON_H
#define CYBER_BUTTON_H
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>


class CyberButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberButton(const QString& text, QWidget* parent = nullptr, bool isPrimary = true)
        : QPushButton(text, parent), m_glowIntensity(0.5), m_isPrimary(isPrimary) {
        setCursor(Qt::PointingHandCursor);
        setStyleSheet("background-color: transparent; border: none; font-family: Consolas; font-size: 9pt; font-weight: bold;");
    }

    double glowIntensity() const { return m_glowIntensity; }
    void setGlowIntensity(double intensity) {
        m_glowIntensity = intensity;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // Paleta kolorów zależna od typu przycisku
        QColor bgColor, borderColor, textColor, glowColor;

        if (m_isPrimary) {
            bgColor = QColor(0, 40, 60);
            borderColor = QColor(0, 200, 255);
            textColor = QColor(0, 220, 255);
            glowColor = QColor(0, 150, 255, 50);
        } else {
            bgColor = QColor(40, 23, 41);
            borderColor = QColor(207, 56, 110);
            textColor = QColor(230, 70, 120);
            glowColor = QColor(200, 50, 100, 50);
        }

        // Ścieżka przycisku ze ściętymi rogami
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

        // Efekt poświaty
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

        // Lewy górny
        painter.drawLine(clipSize + 2, 3, clipSize + 2 + markerSize, 3);
        painter.drawLine(clipSize + 2, 3, clipSize + 2, 3 + markerSize);

        // Prawy górny
        painter.drawLine(width() - clipSize - 2 - markerSize, 3, width() - clipSize - 2, 3);
        painter.drawLine(width() - clipSize - 2, 3, width() - clipSize - 2, 3 + markerSize);

        // Prawy dolny
        painter.drawLine(width() - clipSize - 2 - markerSize, height() - 3, width() - clipSize - 2, height() - 3);
        painter.drawLine(width() - clipSize - 2, height() - 3, width() - clipSize - 2, height() - 3 - markerSize);

        // Lewy dolny
        painter.drawLine(clipSize + 2, height() - 3, clipSize + 2 + markerSize, height() - 3);
        painter.drawLine(clipSize + 2, height() - 3, clipSize + 2, height() - 3 - markerSize);

        // Tekst przycisku
        painter.setPen(QPen(textColor, 1));
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

    void enterEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.9);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QPushButton::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        m_glowIntensity = 1.0;
        update();
        QPushButton::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        m_glowIntensity = 0.9;
        update();
        QPushButton::mouseReleaseEvent(event);
    }

private:
    double m_glowIntensity;
    bool m_isPrimary;
};



#endif //CYBER_BUTTON_H
