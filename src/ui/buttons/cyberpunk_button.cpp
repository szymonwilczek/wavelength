#include "cyberpunk_button.h"
#include <QResizeEvent>
#include <QHBoxLayout>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>

CyberpunkButton::CyberpunkButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent) {
    m_animation = nullptr;

    // Inicjalizacja zmiennych
    m_glowIntensity = 0.0;
    m_frameWidth = 2;
    m_frameColor = QColor(0, 229, 255, 120); // Kolor ramki z mniejszą intensywnością
    m_glowColor = QColor(0, 255, 255, 200); // Kolor poświaty

    // Styl bazowego przycisku (bez underline)
    setStyleSheet(
        "CyberpunkButton {"
        "  background-color: transparent;"
        "  color: #00e5ff;"
        "  border: none;"
        "  padding: 8px 16px;"
        "  font-family: 'Rajdhani', 'Segoe UI', sans-serif;"
        "  font-size: 14px;"
        "  font-weight: 600;"
        "  text-transform: uppercase;"
        "  letter-spacing: 1px;"
        "}"
        "CyberpunkButton:hover {"
        "  color: #00ffff;"
        "}"
        "CyberpunkButton:pressed {"
        "  color: #80ffff;"
        "}"
    );

    // Sprawienie żeby przycisk zawsze zajmował właściwą wielkość dla tekstu
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumWidth(this->fontMetrics().horizontalAdvance(text) + 32);
    setMinimumHeight(36); // Zwiększona wysokość przycisku z 36 na 48

    // Efekt poświaty dla tekstu
    m_glowEffect = new QGraphicsDropShadowEffect(this);
    m_glowEffect->setBlurRadius(10);
    m_glowEffect->setColor(QColor(0, 255, 255, 160));
    m_glowEffect->setOffset(0, 0);
    setGraphicsEffect(m_glowEffect);


    // Animacja propagacji światła
    m_glowAnimation = new QPropertyAnimation(this, "glowIntensity");
    m_glowAnimation->setDuration(600); // Dłuższa animacja dla płynniejszego efektu
    m_glowAnimation->setEasingCurve(QEasingCurve::OutQuad);
}

CyberpunkButton::~CyberpunkButton() {
    if (m_animation) delete m_animation;
    if (m_glowAnimation) delete m_glowAnimation;
}

void CyberpunkButton::paintEvent(QPaintEvent *event) {
    // Najpierw wywołujemy domyślny paintEvent
    QPushButton::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Rysujemy ramkę neonową - jej jasność jest kontrolowana przez m_glowIntensity
    QPen framePen;

    // Kolor ramki z jasnością zależną od intensywności
    QColor borderColor;

    // Bazowy kolor to niebieski neon
    int r = 0;
    int g = 229;
    int b = 255;

    // Alpha (przezroczystość) i grubość zależne od intensywności
    int baseAlpha = 120;
    int maxAlpha = 200;
    int alpha = baseAlpha + (maxAlpha - baseAlpha) * m_glowIntensity;

    // Zwiększamy grubość ramki w zależności od intensywności
    float baseWidth = 2.0;
    float maxWidthIncrease = 0.5; // Maksymalny przyrost grubości
    float penWidth = baseWidth + (m_glowIntensity * maxWidthIncrease);

    borderColor = QColor(r, g, b, alpha);
    framePen.setColor(borderColor);
    framePen.setWidthF(penWidth);

    painter.setPen(framePen);

    // Rysujemy prostokąt z zaokrąglonymi rogami dla ramki
    QRectF frameRect = rect().adjusted(penWidth/2, penWidth/2, -penWidth/2, -penWidth/2);
    painter.drawRoundedRect(frameRect, 5, 5);

    // Dodajemy efekt poświaty dla samej ramki - opcjonalnie
    if (m_glowIntensity > 0.3) { // Tylko przy większej intensywności
        QPen glowPen;
        QColor glowColor = QColor(r, g, b, int(70 * m_glowIntensity));
        glowPen.setColor(glowColor);
        glowPen.setWidthF(penWidth + 0.6);
        painter.setPen(glowPen);

        // Rysujemy drugą, cieńszą ramkę na zewnątrz dla efektu poświaty
        QRectF outerRect = frameRect.adjusted(-0.5, -0.5, 0.5, 0.5);
        painter.drawRoundedRect(outerRect, 5, 5);
    }
}

void CyberpunkButton::setGlowIntensity(qreal intensity) {
    m_glowIntensity = intensity;

    // Aktualizacja efektu poświaty dla tekstu
    if (m_glowEffect) {
        // Zwiększenie blasku i promienia poświaty dla tekstu
        int baseBlur = 7;
        int deltaBlur = 2;
        m_glowEffect->setBlurRadius(baseBlur + deltaBlur * intensity);

        // Zwiększenie jasności poświaty tekstu
        QColor glowColor = QColor(0, 255, 255, 160 + 95 * intensity);
        m_glowEffect->setColor(glowColor);
    }

    update(); // Wymusza przerysowanie przycisku
}

void CyberpunkButton::enterEvent(QEvent *event) {
    QPushButton::enterEvent(event);

    // Animacja rozjaśnienia i propagacji światła
    m_glowAnimation->setStartValue(0.0);
    m_glowAnimation->setEndValue(1.0);
    m_glowAnimation->start();
}

void CyberpunkButton::leaveEvent(QEvent *event) {
    QPushButton::leaveEvent(event);
    // Animacja wygaszenia propagacji światła
    m_glowAnimation->setStartValue(m_glowIntensity);
    m_glowAnimation->setEndValue(0.0);
    m_glowAnimation->start();
}

void CyberpunkButton::resizeEvent(QResizeEvent *event) {
    QPushButton::resizeEvent(event);
}