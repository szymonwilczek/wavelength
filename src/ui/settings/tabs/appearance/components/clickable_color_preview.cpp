#include "clickable_color_preview.h"

ClickableColorPreview::ClickableColorPreview(QWidget *parent): QWidget(parent) {
    setCursor(Qt::PointingHandCursor);
    setFixedSize(24, 24);
    setAutoFillBackground(true); // Pozostaw na true
    // Usuwamy setStyleSheet, bo będziemy rysować ręcznie
    // setStyleSheet("border: 1px solid #555;");
    qDebug() << "ClickableColorPreview created:" << this << " autoFillBackground:" << autoFillBackground();
}

void ClickableColorPreview::setColor(const QColor &color) {
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

void ClickableColorPreview::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        qDebug() << "ClickableColorPreview clicked:" << this;
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void ClickableColorPreview::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);

    // Pobierz kolor tła z palety (teraz powinien być poprawny)
    const QColor bgColor = palette().color(QPalette::Window);

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
