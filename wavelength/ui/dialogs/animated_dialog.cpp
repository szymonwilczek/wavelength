#include "animated_dialog.h"
#include <QPropertyAnimation>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QMainWindow>

// Implementacja OverlayWidget z optymalizacjami
OverlayWidget::OverlayWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents); // Pozwala klikać przez overlay
    setAttribute(Qt::WA_TranslucentBackground);     // Przezroczyste tło
    setAttribute(Qt::WA_OpaquePaintEvent, false);   // Nie wymaga pełnego odświeżenia
    setAttribute(Qt::WA_NoSystemBackground, true);  // Brak systemowego tła
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // Wyłączamy antialiasing dla wydajności

    // Półprzezroczysty kolor tła - ustaw alpha na poziomie, który zapewni odpowiednie przyciemnienie
    // ale wciąż pozwoli zobaczyć tło aplikacji
    QColor overlayColor(0, 0, 0, 120); // Kolor czarny z alpha 120/255 (ok. 47% przezroczystości)
    painter.fillRect(rect(), overlayColor);
}

// Implementacja AnimatedDialog
AnimatedDialog::AnimatedDialog(QWidget *parent, AnimationType type)
    : QDialog(parent),
      m_animationType(type),
      m_duration(300),
      m_closing(false),
      m_overlay(nullptr)
{
    // Niezbędne dla animacji
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

AnimatedDialog::~AnimatedDialog()
{
    // Upewniamy się, że overlay zostanie usunięty
    if (m_overlay) {
        m_overlay->deleteLater();
    }
}

void AnimatedDialog::showEvent(QShowEvent *event)
{
    // Tworzenie overlay'a dla przyciemnienia tła
    QWidget *parent_widget = QDialog::parentWidget();
    if (!parent_widget) {
        parent_widget = QApplication::activeWindow();
    }

    if (parent_widget) {
        m_overlay = new OverlayWidget(parent_widget);
        m_overlay->setGeometry(parent_widget->rect());
        m_overlay->show();

        // Zapewnienie, że dialog będzie zawsze na wierzchu
        raise();
    }

    event->accept();
    animateShow();
}

void AnimatedDialog::closeEvent(QCloseEvent *event)
{
    if (!m_closing) {
        event->ignore();
        m_closing = true;
        animateClose();
    } else {
        // Usuwamy overlay przy zamykaniu
        if (m_overlay) {
            m_overlay->deleteLater();
            m_overlay = nullptr;
        }
        event->accept();
    }
}

void AnimatedDialog::animateShow()
{
    QPropertyAnimation *animation = nullptr;

    // Przechowujemy początkową geometrię
    QRect geometry = this->geometry();

    switch (m_animationType) {
        case SlideFromBottom: {
            // Rozpoczynamy poza ekranem na dole
            QRect startGeometry = geometry;
            startGeometry.moveBottom(QGuiApplication::screenAt(geometry.center())->geometry().bottom() + 50);
            this->setGeometry(startGeometry);

            // Animacja przesunięcia
            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(startGeometry);
            animation->setEndValue(geometry);
            animation->setEasingCurve(QEasingCurve::OutQuint);
            break;
        }

        case FadeIn: {
            // Efekt przezroczystości
            QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(this);
            this->setGraphicsEffect(effect);
            effect->setOpacity(0);

            animation = new QPropertyAnimation(effect, "opacity");
            animation->setStartValue(0.0);
            animation->setEndValue(1.0);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            break;
        }

        case ScaleFromCenter: {
            // Rozpoczynamy z małego punktu na środku
            QPoint center = geometry.center();
            QRect startGeometry(center.x(), center.y(), 0, 0);
            startGeometry.moveCenter(center);
            this->setGeometry(startGeometry);

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(startGeometry);
            animation->setEndValue(geometry);
            animation->setEasingCurve(QEasingCurve::OutBack);
            break;
        }
    }

    if (animation) {
        animation->setDuration(m_duration);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

void AnimatedDialog::animateClose()
{
    QPropertyAnimation *animation = nullptr;

    // Przechowujemy końcową geometrię (poza ekranem)
    QRect endGeometry = this->geometry();

    switch (m_animationType) {
        case SlideFromBottom: {
            // Przesuwamy poza ekran na dole
            endGeometry.moveTop(QGuiApplication::screenAt(endGeometry.center())->geometry().bottom());

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(this->geometry());
            animation->setEndValue(endGeometry);
            animation->setEasingCurve(QEasingCurve::InQuint);
            break;
        }

        case FadeIn: {
            // Efekt przezroczystości powinien już być ustawiony z animateShow()
            QGraphicsOpacityEffect *effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect());
            if (!effect) {
                effect = new QGraphicsOpacityEffect(this);
                this->setGraphicsEffect(effect);
            }

            animation = new QPropertyAnimation(effect, "opacity");
            animation->setStartValue(1.0);
            animation->setEndValue(0.0);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            break;
        }

        case ScaleFromCenter: {
            // Kurczymy do punktu na środku
            QPoint center = this->geometry().center();
            QRect startGeometry = this->geometry();
            QRect endGeometry(0, 0, 0, 0);
            endGeometry.moveCenter(center);

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(startGeometry);
            animation->setEndValue(endGeometry);
            animation->setEasingCurve(QEasingCurve::InBack);
            break;
        }
    }

    if (animation) {
        animation->setDuration(m_duration);
        connect(animation, &QPropertyAnimation::finished, this, [this]() {
            QDialog::close();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}