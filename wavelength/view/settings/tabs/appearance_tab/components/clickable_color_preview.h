#ifndef CLICKABLE_COLOR_PREVIEW_H
#define CLICKABLE_COLOR_PREVIEW_H

#include <QWidget>
#include <QMouseEvent>
#include <QColor>
#include <QString>
#include <QDebug>
#include <QStyleOption>
#include <QPainter>

class ClickableColorPreview : public QWidget {
    Q_OBJECT

public:
    explicit ClickableColorPreview(QWidget *parent = nullptr) : QWidget(parent) {
        setCursor(Qt::PointingHandCursor);
        setFixedSize(24, 24);
        setAutoFillBackground(true); // Pozostaw na true
        // Usuwamy setStyleSheet, bo będziemy rysować ręcznie
        // setStyleSheet("border: 1px solid #555;");
        qDebug() << "ClickableColorPreview created:" << this << " autoFillBackground:" << autoFillBackground();
    }

public slots:
    void setColor(const QColor& color) {
        qDebug() << "ClickableColorPreview::setColor called on" << this << "with color" << color.name();
        // m_currentColor = color; // Nie potrzebujemy już m_currentColor, jeśli używamy palety

        // Ustaw poprawny kolor w palecie
        QPalette pal = palette();
        if (color.isValid()) {
             pal.setColor(QPalette::Window, color); // <<< Użyj przekazanego koloru!
        } else {
             pal.setColor(QPalette::Window, Qt::transparent); // Ustaw przezroczysty dla nieprawidłowego
        }
        setPalette(pal); // Zastosuj zaktualizowaną paletę

        update(); // Wymuś odświeżenie (wywoła paintEvent)
    }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            qDebug() << "ClickableColorPreview clicked:" << this;
            emit clicked();
        }
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);
        QPainter painter(this);

        // Pobierz kolor tła z palety (teraz powinien być poprawny)
        QColor bgColor = palette().color(QPalette::Window);

        qDebug() << "ClickableColorPreview::paintEvent for" << this << "Drawing background:" << bgColor.name();

        // Rysuj tło
        if (bgColor.isValid() && bgColor.alpha() > 0) {
            painter.fillRect(rect(), bgColor);
        } else {
            // Opcjonalnie: Narysuj coś, jeśli tło jest przezroczyste/nieprawidłowe
            // painter.fillRect(rect(), Qt::lightGray); // Np. szare tło
        }

        // Rysuj ramkę ręcznie
        painter.setPen(QPen(QColor("#555555"), 1)); // Ustaw kolor i grubość ramki
        // Rysuj prostokąt wewnątrz granic widgetu, uwzględniając grubość pióra
        // adjusted(0, 0, -1, -1) zapewnia, że ramka o grubości 1px mieści się w całości
        painter.drawRect(rect().adjusted(0, 0, -1, -1));

    }

// private:
    // QColor m_currentColor; // Już niepotrzebne
};

#endif //CLICKABLE_COLOR_PREVIEW_H