#include "cyber_line_edit.h"

CyberLineEdit::CyberLineEdit(QWidget *parent): QLineEdit(parent), m_glowIntensity(0.0), m_cursorVisible(true) {
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
    m_cursorBlinkTimer = new QTimer(this);
    m_cursorBlinkTimer->setInterval(530); // Standardowy czas mrugania kursora
    connect(m_cursorBlinkTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;

        // Wymuszamy odświeżenie tylko obszaru kursora dla lepszej wydajności
        if (hasFocus()) {
            QRect cursorRect = this->cursorRect();
            cursorRect.adjust(-2, -2, 2, 2); // Dodajemy mały margines
            update(cursorRect);
        }
    });
}

CyberLineEdit::~CyberLineEdit() {
    if (m_cursorBlinkTimer) {
        m_cursorBlinkTimer->stop();
    }
}

QSize CyberLineEdit::sizeHint() const {
    QSize size = QLineEdit::sizeHint();
    size.setHeight(30); // Wymuszamy wysokość 30px
    return size;
}

void CyberLineEdit::setGlowIntensity(double intensity) {
    m_glowIntensity = intensity;
    update();
}

QRect CyberLineEdit::cursorRect() const {
    // Obliczamy pozycję kursora
    int cursorX = 10;
    QString content = text();

    if (!content.isEmpty()) {
        QString textBeforeCursor = content.left(cursorPosition());
        QFontMetrics fm(font());
        cursorX += fm.horizontalAdvance(echoMode() == QLineEdit::Password ?
                                            QString(textBeforeCursor.length(), '•') :
                                            textBeforeCursor);
    }

    int cursorHeight = height() * 0.75;
    int cursorY = (height() - cursorHeight) / 2;

    return QRect(cursorX - 1, cursorY, 2, cursorHeight);
}

void CyberLineEdit::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Kolory
    QColor bgColor(0, 24, 34);
    QColor borderColor(0, 180, 255);

    // Tworzymy ścieżkę ze ściętymi rogami
    QPainterPath path;
    int clipSize = 6; // rozmiar ścięcia
    int w = width();
    int h = height();

    path.moveTo(clipSize, 0);
    path.lineTo(w - clipSize, 0);
    path.lineTo(w, clipSize);
    path.lineTo(w, h - clipSize);
    path.lineTo(w - clipSize, h);
    path.lineTo(clipSize, h);
    path.lineTo(0, h - clipSize);
    path.lineTo(0, clipSize);
    path.closeSubpath();

    // Tło
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawPath(path);

    // Obramowanie
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // Efekt świecenia przy fokusie
    if (hasFocus() || m_glowIntensity > 0.1) {
        double intensity = hasFocus() ? 1.0 : m_glowIntensity;

        QColor glowColor = borderColor;
        glowColor.setAlpha(80 * intensity);

        painter.setPen(QPen(glowColor, 2.0));
        painter.drawPath(path);
    }

    // Efekt technologicznych linii
    painter.setPen(QPen(QColor(0, 100, 150), 1, Qt::DotLine));
    painter.drawLine(clipSize * 2, h - 2, w - clipSize * 2, h - 2);

    // Rysowanie tekstu
    QRect textRect = rect().adjusted(10, 0, -10, 0);

    // Parametry tekstu
    QString content = text();
    QString placeholder = placeholderText();

    if (content.isEmpty() && !hasFocus() && !placeholder.isEmpty()) {
        painter.setPen(QPen(QColor(0, 140, 180)));
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, placeholder);
    } else {
        painter.setPen(QPen(QColor(0, 220, 255)));

        if (echoMode() == QLineEdit::Password) {
            content = QString(content.length(), '•');
        }

        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, content);
    }

    // Poprawiona obsługa kursora - konsekwentna wysokość i pozycja
    if (hasFocus() && m_cursorVisible) {
        int cursorX = 10; // Domyślna pozycja X

        // Oblicz szerokość tekstu przed kursorem
        if (!content.isEmpty()) {
            QString textBeforeCursor = content.left(cursorPosition());
            QFontMetrics fm(font());
            cursorX += fm.horizontalAdvance(echoMode() == QLineEdit::Password ?
                                                QString(textBeforeCursor.length(), '•') :
                                                textBeforeCursor);
        }

        // Stała wysokość kursora (75% wysokości pola)
        int cursorHeight = height() * 0.75;
        int cursorY = (height() - cursorHeight) / 2;

        painter.setPen(QPen(QColor(0, 220, 255), 1));
        painter.drawLine(QPoint(cursorX, cursorY), QPoint(cursorX, cursorY + cursorHeight));
    }
}

void CyberLineEdit::focusInEvent(QFocusEvent *event) {
    m_cursorVisible = true;

    // Uruchom timer mrugania kursora
    if (m_cursorBlinkTimer && !m_cursorBlinkTimer->isActive()) {
        m_cursorBlinkTimer->start();
    }

    QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QPropertyAnimation::DeleteWhenStopped);

    QLineEdit::focusInEvent(event);
}

void CyberLineEdit::focusOutEvent(QFocusEvent *event) {
    // Zatrzymaj timer mrugania kursora
    if (m_cursorBlinkTimer && m_cursorBlinkTimer->isActive()) {
        m_cursorBlinkTimer->stop();
    }

    QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
    anim->setDuration(300);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    anim->start(QPropertyAnimation::DeleteWhenStopped);

    QLineEdit::focusOutEvent(event);
}

void CyberLineEdit::enterEvent(QEvent *event) {
    if (!hasFocus()) {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.5);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    QLineEdit::enterEvent(event);
}

void CyberLineEdit::leaveEvent(QEvent *event) {
    if (!hasFocus()) {
        QPropertyAnimation* anim = new QPropertyAnimation(this, "glowIntensity");
        anim->setDuration(200);
        anim->setStartValue(m_glowIntensity);
        anim->setEndValue(0.0);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    }
    QLineEdit::leaveEvent(event);
}

void CyberLineEdit::keyPressEvent(QKeyEvent *event) {
    // Resetuj stan mrugania po każdym naciśnięciu klawisza
    m_cursorVisible = true;

    // Resetuj timer mrugania
    if (m_cursorBlinkTimer && m_cursorBlinkTimer->isActive()) {
        m_cursorBlinkTimer->start();
    }

    QLineEdit::keyPressEvent(event);
}
