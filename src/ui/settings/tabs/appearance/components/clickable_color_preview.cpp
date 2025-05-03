#include "clickable_color_preview.h"

ClickableColorPreview::ClickableColorPreview(QWidget *parent): QWidget(parent) {
    setCursor(Qt::PointingHandCursor);
    setFixedSize(24, 24);
    setAutoFillBackground(true); // Pozostaw na true
    // Usuwamy setStyleSheet, bo będziemy rysować ręcznie
    // setStyleSheet("border: 1px solid #555;");
}

void ClickableColorPreview::SetColor(const QColor &color) {

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
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void ClickableColorPreview::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);

    // Pobierz kolor tła z palety (teraz powinien być poprawny)
    const QColor background_color = palette().color(QPalette::Window);

    // Rysuj tło
    if (background_color.isValid() && background_color.alpha() > 0) {
        painter.fillRect(rect(), background_color);
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
