#include "animated_dialog.h"
#include <QPropertyAnimation>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QMainWindow>

#include "../../dialogs/wavelength_dialog.h"

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

void AnimatedDialog::initScanlineBuffer() {
    if (!m_scanlineInitialized) {
        // Utwórz bufor o stałej wysokości, ale pełnej szerokości
        m_scanlineBuffer = QPixmap(width(), 20);
        m_scanlineBuffer.fill(Qt::transparent);

        // Utwórz gradient raz
        m_scanGradient = QLinearGradient(0, 0, 0, 20);
        m_scanGradient.setColorAt(0, QColor(0, 200, 255, 0));
        m_scanGradient.setColorAt(0.5, QColor(0, 220, 255, 180));
        m_scanGradient.setColorAt(1, QColor(0, 200, 255, 0));

        // Rysuj gradient do bufora
        QPainter bufferPainter(&m_scanlineBuffer);
        bufferPainter.setPen(Qt::NoPen);
        bufferPainter.setBrush(m_scanGradient);
        bufferPainter.drawRect(0, 0, width(), 20);

        m_scanlineInitialized = true;
    }
}

void AnimatedDialog::animateShow()
{
    QPropertyAnimation *animation = nullptr;
    QAbstractAnimation* finalAnimation = nullptr;

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
            finalAnimation = animation;
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
                finalAnimation = animation;
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
                finalAnimation = animation;
            break;
        }

// Zmodyfikuj przypadek DigitalMaterialization w funkcji animateShow()
case DigitalMaterialization: {
    // Zamiast animować całą geometrię, co wymusza pełne przerysowania,
    // animuj tylko pozycję Y, co jest mniej kosztowne
    int startY = geometry.y() - 150;
    int endY = geometry.y();

    // Ustaw dialog na początkowej pozycji
    this->move(geometry.x(), startY);

    // Animacja przesunięcia tylko współrzędnej Y
    QPropertyAnimation* posAnim = new QPropertyAnimation(this, "pos");
    posAnim->setStartValue(QPoint(geometry.x(), startY));
    posAnim->setEndValue(QPoint(geometry.x(), endY));
    posAnim->setEasingCurve(QEasingCurve::OutQuad); // Lżejsza krzywa
    posAnim->setDuration(m_duration);

    // Optymalizacja flagi dla systemu kompozycji
    this->setWindowOpacity(1.0); // Zapewnia że system kompozycji przejmie animację

    // Dostęp do WavelengthDialog dla dodatkowych efektów
    if (auto digitalDialog = qobject_cast<WavelengthDialog*>(this)) {
        // Animacja digitalizacji z kontrolowanym odświeżaniem
        QPropertyAnimation* digitAnim = new QPropertyAnimation(digitalDialog, "digitalizationProgress");
        digitAnim->setStartValue(0.0);
        digitAnim->setEndValue(1.0);
        digitAnim->setEasingCurve(QEasingCurve::InOutQuad);
        digitAnim->setDuration(m_duration * 1.5); // Nieco krótsza

        // Zmniejszamy liczbę jednoczesnych animacji - usuwamy animację rogów
        // która jest najmniej istotna wizualnie

        // Grupa animacji
        QSequentialAnimationGroup* group = new QSequentialAnimationGroup();
        // Najpierw animujemy pozycję
        group->addAnimation(posAnim);

        // Potem efekty digitalizacji (zapobiega nakładaniu się obliczeń)
        QParallelAnimationGroup* effectsGroup = new QParallelAnimationGroup();
        effectsGroup->addAnimation(digitAnim);
        group->addAnimation(effectsGroup);

        connect(group, &QAbstractAnimation::finished, this, [this]() {
            emit showAnimationFinished();
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
        finalAnimation = group;
    }
    break;
}
    }

    if (finalAnimation) {
        // Ustawiamy czas trwania jeśli to pojedyncza animacja
        if (auto propAnim = qobject_cast<QPropertyAnimation*>(finalAnimation)) {
            propAnim->setDuration(m_duration);
        }

        // Podłączamy sygnał zakończenia do JEDNEGO miejsca
        connect(finalAnimation, &QAbstractAnimation::finished, this, [this]() {
            emit showAnimationFinished(); // Emitujemy sygnał po zakończeniu wszystkich animacji
        });

        // Jeśli to nie grupa animacji (która już została uruchomiona), startujemy animację
        if (!qobject_cast<QParallelAnimationGroup*>(finalAnimation)) {
            finalAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        }
    } else {
        // Jeśli żadna animacja nie jest tworzona, emitujemy sygnał od razu
        emit showAnimationFinished();
    }
}

void AnimatedDialog::animateClose()
{
    QPropertyAnimation *animation = nullptr;
    QAbstractAnimation* finalAnimation = nullptr;

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

        case DigitalMaterialization: {
    // Animacja wyjazdu do góry
    QRect endGeometry = this->geometry();
    endGeometry.moveTop(endGeometry.top() - 100);

    animation = new QPropertyAnimation(this, "geometry");
    animation->setStartValue(this->geometry());
    animation->setEndValue(endGeometry);
    animation->setEasingCurve(QEasingCurve::InQuint);

    // Dostęp do WavelengthDialog dla dodatkowych efektów
    if (auto digitalDialog = qobject_cast<WavelengthDialog*>(this)) {
        // Animacja digitalizacji w odwrotną stronę
        QPropertyAnimation* digitAnim = new QPropertyAnimation(digitalDialog, "digitalizationProgress");
        digitAnim->setStartValue(digitalDialog->digitalizationProgress());
        digitAnim->setEndValue(0.0);
        digitAnim->setDuration(m_duration);

        // Końcowy efekt glitch
        QPropertyAnimation* glitchAnim = new QPropertyAnimation(digitalDialog, "glitchIntensity");
        glitchAnim->setStartValue(0.0);
        glitchAnim->setEndValue(0.0);
        glitchAnim->setKeyValueAt(0.3, 0.8); // Intensywny glitch w trakcie zamykania
        glitchAnim->setDuration(m_duration);

        // Grupa animacji
        QParallelAnimationGroup* group = new QParallelAnimationGroup();
        group->addAnimation(animation);
        group->addAnimation(digitAnim);
        group->addAnimation(glitchAnim);

        connect(group, &QAbstractAnimation::finished, this, [this]() {
            QDialog::close();
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
        finalAnimation = group;
    }
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