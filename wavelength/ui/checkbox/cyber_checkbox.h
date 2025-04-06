//
// Created by szymo on 6.04.2025.
//

#ifndef CYBER_CHECKBOX_H
#define CYBER_CHECKBOX_H
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QStyleOptionButton>


class CyberCheckBox : public QCheckBox {
    Q_OBJECT
    Q_PROPERTY(double glowIntensity READ glowIntensity WRITE setGlowIntensity)

public:
    CyberCheckBox(const QString& text, QWidget* parent = nullptr)
        : QCheckBox(text, parent), m_glowIntensity(0.5) {
        // Dodaj marginesy, aby zapewnić poprawny rozmiar
        setStyleSheet("QCheckBox { spacing: 8px; background-color: transparent; color: #00ccff; font-family: Consolas; font-size: 9pt; margin-top: 4px; margin-bottom: 4px; }");

        // Ustaw minimalną wysokość dla checkboxa
        setMinimumHeight(24);
    }

    QSize sizeHint() const override {
        QSize size = QCheckBox::sizeHint();
        size.setHeight(qMax(size.height(), 24)); // Minimum 24px wysokości
        return size;
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

        // Nie rysujemy standardowego wyglądu
        QStyleOptionButton opt;
        opt.initFrom(this);

        // Kolory
        QColor bgColor(0, 30, 40);
        QColor borderColor(0, 200, 255);
        QColor checkColor(0, 220, 255);
        QColor textColor(0, 200, 255);

        // Rysowanie pola wyboru (kwadrat ze ściętymi rogami)
        const int checkboxSize = 16;
        const int x = 0;
        const int y = (height() - checkboxSize) / 2;

        // Ścieżka dla pola wyboru ze ściętymi rogami
        QPainterPath path;
        int clipSize = 3; // rozmiar ścięcia

        path.moveTo(x + clipSize, y);
        path.lineTo(x + checkboxSize - clipSize, y);
        path.lineTo(x + checkboxSize, y + clipSize);
        path.lineTo(x + checkboxSize, y + checkboxSize - clipSize);
        path.lineTo(x + checkboxSize - clipSize, y + checkboxSize);
        path.lineTo(x + clipSize, y + checkboxSize);
        path.lineTo(x, y + checkboxSize - clipSize);
        path.lineTo(x, y + clipSize);
        path.closeSubpath();

        // Tło pola
        painter.setPen(Qt::NoPen);
        painter.setBrush(bgColor);
        painter.drawPath(path);

        // Obramowanie
        painter.setPen(QPen(borderColor, 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        // Efekt poświaty
        if (m_glowIntensity > 0.1) {
            QColor glowColor = borderColor;
            glowColor.setAlpha(80 * m_glowIntensity);

            painter.setPen(QPen(glowColor, 2.0));
            painter.drawPath(path);
        }

        // Rysowanie znacznika (jeśli zaznaczony)
        if (isChecked()) {
            // Znacznik w postaci "X" w stylu technologicznym
            painter.setPen(QPen(checkColor, 2.0));
            int margin = 3;
            painter.drawLine(x + margin, y + margin, x + checkboxSize - margin, y + checkboxSize - margin);
            painter.drawLine(x + checkboxSize - margin, y + margin, x + margin, y + checkboxSize - margin);
        }

        // Skalowanie tekstu
        QFont font = painter.font();
        font.setFamily("Blender Pro Book");
        font.setPointSize(9);
        painter.setFont(font);

        // Rysowanie tekstu
        QRect textRect(x + checkboxSize + 5, 0, width() - checkboxSize - 5, height());
        painter.setPen(textColor);
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text());
    }

    void enterEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.9);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QCheckBox::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
        QCheckBox::leaveEvent(event);
    }

private:
    double m_glowIntensity;
};



#endif //CYBER_CHECKBOX_H
