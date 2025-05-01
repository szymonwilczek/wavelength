#include "cyber_line_edit.h"

#include <QPropertyAnimation>

CyberLineEdit::CyberLineEdit(QWidget *parent): QLineEdit(parent), glow_intensity_(0.0), cursor_visible_(true) {
    // Zwiększ padding oraz ustaw minimalny rozmiar
    setStyleSheet("border: none; background-color: transparent; padding: 6px; font-family: Consolas; font-size: 9pt;");
    setCursor(Qt::IBeamCursor);

    // Ustawienie minimalnej wysokości zapewnia poprawny rendering od początku
    setMinimumHeight(30);

    // Kolor tekstu
    QPalette pal = palette();
    pal.setColor(QPalette::Text, QColor(0, 220, 255));
    setPalette(pal);

    // Własny timer mrugania kursora
    cursor_blink_timer_ = new QTimer(this);
    cursor_blink_timer_->setInterval(530); // Standardowy czas mrugania kursora
    connect(cursor_blink_timer_, &QTimer::timeout, this, [this]() {
        cursor_visible_ = !cursor_visible_;

        // Wymuszamy odświeżenie tylko obszaru kursora dla lepszej wydajności
        if (hasFocus()) {
            QRect cursorRect = this->CyberCursorRect();
            cursorRect.adjust(-2, -2, 2, 2); // Dodajemy mały margines
            update(cursorRect);
        }
    });
}

CyberLineEdit::~CyberLineEdit() {
    if (cursor_blink_timer_) {
        cursor_blink_timer_->stop();
    }
}

QSize CyberLineEdit::sizeHint() const {
    QSize size = QLineEdit::sizeHint();
    size.setHeight(30); // Wymuszamy wysokość 30px
    return size;
}

void CyberLineEdit::SetGlowIntensity(const double intensity) {
    glow_intensity_ = intensity;
    update();
}

QRect CyberLineEdit::CyberCursorRect() const {
    // Obliczamy pozycję kursora
    int cursor_x = 10;
    const QString content = text();

    if (!content.isEmpty()) {
        const QString text_before_cursor = content.left(cursorPosition());
        const QFontMetrics font_metrics(font());
        cursor_x += font_metrics.horizontalAdvance(echoMode() == QLineEdit::Password ?
                                            QString(text_before_cursor.length(), '•') :
                                            text_before_cursor);
    }

    const int cursor_height = height() * 0.75;
    const int cursor_y = (height() - cursor_height) / 2;

    return QRect(cursor_x - 1, cursor_y, 2, cursor_height);
}

void CyberLineEdit::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Kolory
    QColor background_color(0, 24, 34);
    QColor border_color(0, 180, 255);

    // Tworzymy ścieżkę ze ściętymi rogami
    QPainterPath path;
    int clip_size = 6; // rozmiar ścięcia
    int w = width();
    int h = height();

    path.moveTo(clip_size, 0);
    path.lineTo(w - clip_size, 0);
    path.lineTo(w, clip_size);
    path.lineTo(w, h - clip_size);
    path.lineTo(w - clip_size, h);
    path.lineTo(clip_size, h);
    path.lineTo(0, h - clip_size);
    path.lineTo(0, clip_size);
    path.closeSubpath();

    // Tło
    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(border_color, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Efekt świecenia przy fokusie
    if (hasFocus() || glow_intensity_ > 0.1) {
        double intensity = hasFocus() ? 1.0 : glow_intensity_;

        QColor glow_color = border_color;
        glow_color.setAlpha(80 * intensity);

        painter.setPen(QPen(glow_color, 2.0));
        painter.drawPath(path);
    }

    // Efekt technologicznych linii
    painter.setPen(QPen(QColor(0, 100, 150), 1, Qt::DotLine));
    painter.drawLine(clip_size * 2, h - 2, w - clip_size * 2, h - 2);

    // Rysowanie tekstu
    QRect text_rect = rect().adjusted(10, 0, -10, 0);

    // Parametry tekstu
    QString content = text();
    QString placeholder = placeholderText();

    if (content.isEmpty() && !hasFocus() && !placeholder.isEmpty()) {
        painter.setPen(QPen(QColor(0, 140, 180)));
        painter.drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, placeholder);
    } else {
        painter.setPen(QPen(QColor(0, 220, 255)));

        if (echoMode() == Password) {
            content = QString(content.length(), '•');
        }

        painter.drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, content);
    }

    // Poprawiona obsługa kursora - konsekwentna wysokość i pozycja
    if (hasFocus() && cursor_visible_) {
        int cursor_x = 10; // Domyślna pozycja X

        // Oblicz szerokość tekstu przed kursorem
        if (!content.isEmpty()) {
            QString text_before_cursor = content.left(cursorPosition());
            QFontMetrics font_metrics(font());
            cursor_x += font_metrics.horizontalAdvance(echoMode() == Password ?
                                                QString(text_before_cursor.length(), '•') :
                                                text_before_cursor);
        }

        // Stała wysokość kursora (75% wysokości pola)
        int cursor_height = height() * 0.75;
        int cursor_y = (height() - cursor_height) / 2;

        painter.setPen(QPen(QColor(0, 220, 255), 1));
        painter.drawLine(QPoint(cursor_x, cursor_y), QPoint(cursor_x, cursor_y + cursor_height));
    }
}

void CyberLineEdit::focusInEvent(QFocusEvent *event) {
    cursor_visible_ = true;

    // Uruchom timer mrugania kursora
    if (cursor_blink_timer_ && !cursor_blink_timer_->isActive()) {
        cursor_blink_timer_->start();
    }

    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(200);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->start(QPropertyAnimation::DeleteWhenStopped);

    QLineEdit::focusInEvent(event);
}

void CyberLineEdit::focusOutEvent(QFocusEvent *event) {
    // Zatrzymaj timer mrugania kursora
    if (cursor_blink_timer_ && cursor_blink_timer_->isActive()) {
        cursor_blink_timer_->stop();
    }

    const auto animation = new QPropertyAnimation(this, "glowIntensity");
    animation->setDuration(300);
    animation->setStartValue(1.0);
    animation->setEndValue(0.0);
    animation->start(QPropertyAnimation::DeleteWhenStopped);

    QLineEdit::focusOutEvent(event);
}

void CyberLineEdit::enterEvent(QEvent *event) {
    if (!hasFocus()) {
        const auto animation = new QPropertyAnimation(this, "glowIntensity");
        animation->setDuration(200);
        animation->setStartValue(glow_intensity_);
        animation->setEndValue(0.5);
        animation->start(QPropertyAnimation::DeleteWhenStopped);
    }
    QLineEdit::enterEvent(event);
}

void CyberLineEdit::leaveEvent(QEvent *event) {
    if (!hasFocus()) {
        const auto animation = new QPropertyAnimation(this, "glowIntensity");
        animation->setDuration(200);
        animation->setStartValue(glow_intensity_);
        animation->setEndValue(0.0);
        animation->start(QPropertyAnimation::DeleteWhenStopped);
    }
    QLineEdit::leaveEvent(event);
}

void CyberLineEdit::keyPressEvent(QKeyEvent *event) {
    // Resetuj stan mrugania po każdym naciśnięciu klawisza
    cursor_visible_ = true;

    // Resetuj timer mrugania
    if (cursor_blink_timer_ && cursor_blink_timer_->isActive()) {
        cursor_blink_timer_->start();
    }

    QLineEdit::keyPressEvent(event);
}
