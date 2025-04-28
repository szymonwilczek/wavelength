#include "animated_dialog.h"
#include <QScreen>

#include "../../ui/dialogs/join_wavelength_dialog.h"
#include "../../ui/dialogs/wavelength_dialog.h"
#include "../navigation/navbar.h"
#include "../widgets/overlay_widget.h"

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
    QWidget *parent_widget = QDialog::parentWidget();
    if (!parent_widget) {
        parent_widget = QApplication::activeWindow();
    }

    if (parent_widget) {
        if (!m_overlay) {
            m_overlay = new OverlayWidget(parent_widget);
        }

        // Animacja pokazywania overlay'a
        QPropertyAnimation *overlayAnim = new QPropertyAnimation(m_overlay, "opacity");
        overlayAnim->setDuration(200);
        overlayAnim->setStartValue(0.0);
        overlayAnim->setEndValue(1.0);
        overlayAnim->setEasingCurve(QEasingCurve::InOutQuad);

        m_overlay->updateGeometry(parent_widget->rect());
        m_overlay->show();
        overlayAnim->start(QAbstractAnimation::DeleteWhenStopped);

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

        if (m_overlay) {
            // Animacja ukrywania overlay'a
            QPropertyAnimation *overlayAnim = new QPropertyAnimation(m_overlay, "opacity");
            overlayAnim->setDuration(200);
            overlayAnim->setStartValue(m_overlay->opacity());
            overlayAnim->setEndValue(0.0);
            overlayAnim->setEasingCurve(QEasingCurve::InOutQuad);

            connect(overlayAnim, &QPropertyAnimation::finished, this, [this]() {
                if (m_overlay) {
                    m_overlay->deleteLater();
                    m_overlay = nullptr;
                }
            });

            overlayAnim->start(QAbstractAnimation::DeleteWhenStopped);
        }

        animateClose();
    } else {
        event->accept();
    }
}



void AnimatedDialog::animateShow()
{
    QPropertyAnimation *animation = nullptr;
    QAbstractAnimation* finalAnimation = nullptr;

    // Przechowujemy początkową geometrię
    QRect geometry = this->geometry();

    switch (m_animationType) {
        // ... (inne przypadki animacji bez zmian) ...

        // Zmodyfikuj przypadek DigitalMaterialization w funkcji animateShow()
        case DigitalMaterialization: {
            // --- ZMIANA: Połączenie animacji pozycji i przezroczystości ---
            // Ustaw początkową pozycję i przezroczystość
            int startY = geometry.y() - 100; // Zmniejszono nieco dystans początkowy dla subtelniejszego efektu
            int endY = geometry.y();
            this->move(geometry.x(), startY);
            this->setWindowOpacity(0.0); // Start fully transparent

            // 1a. Animacja pozycji (wjazd z góry)
            QPropertyAnimation* posAnim = new QPropertyAnimation(this, "pos");
            posAnim->setStartValue(QPoint(geometry.x(), startY));
            posAnim->setEndValue(QPoint(geometry.x(), endY));
            posAnim->setEasingCurve(QEasingCurve::OutCubic); // Nieco płynniejsza krzywa niż OutQuad
            posAnim->setDuration(m_duration); // Czas trwania animacji pozycji

            // 1b. Animacja przezroczystości (płynne pojawienie)
            QPropertyAnimation* opacityAnim = new QPropertyAnimation(this, "windowOpacity");
            opacityAnim->setStartValue(0.0);
            opacityAnim->setEndValue(1.0);
            opacityAnim->setEasingCurve(QEasingCurve::OutCubic); // Ta sama krzywa dla spójności
            opacityAnim->setDuration(m_duration); // Ten sam czas trwania

            // Grupa równoległa dla jednoczesnego wjazdu i pojawienia się
            QParallelAnimationGroup* showParallelGroup = new QParallelAnimationGroup(this);
            showParallelGroup->addAnimation(posAnim);
            showParallelGroup->addAnimation(opacityAnim);
            // --- KONIEC ZMIANY 1 ---

            // --- ZMIANA 2: Użycie QSequentialAnimationGroup z grupą równoległą ---
            QSequentialAnimationGroup* sequentialGroup = new QSequentialAnimationGroup(this);
            sequentialGroup->addAnimation(showParallelGroup); // 1. Animacja wjazdu i pojawienia się (równoległa)

            // Sprawdzamy typ dialogu i dodajemy animację digitalizacji *po* animacji wjazdu/pojawienia
            bool isDialogAnimated = false;
            QPropertyAnimation* digitAnim = nullptr;

            // Obsługa WavelengthDialog
            if (auto digitalDialog = qobject_cast<WavelengthDialog*>(this)) {
                isDialogAnimated = true;
                digitalDialog->startRefreshTimer();

                digitAnim = new QPropertyAnimation(digitalDialog, "digitalizationProgress", sequentialGroup);
                digitAnim->setStartValue(0.0);
                digitAnim->setEndValue(1.0);
                digitAnim->setEasingCurve(QEasingCurve::InOutQuad);
                digitAnim->setDuration(m_duration * 1.5);

                sequentialGroup->addAnimation(digitAnim); // 2. Animacja digitalizacji

                connect(sequentialGroup, &QSequentialAnimationGroup::finished, this, [this, digitalDialog]() {
                    emit showAnimationFinished();
                    digitalDialog->setRefreshTimerInterval(100);
                    qDebug() << "LOG: Sekwencja animacji DigitalMaterialization zakończona (WavelengthDialog)";
                });
            }
            // Obsługa JoinWavelengthDialog
            else if (auto joinDialog = qobject_cast<JoinWavelengthDialog*>(this)) {
                isDialogAnimated = true;
                joinDialog->startRefreshTimer();

                digitAnim = new QPropertyAnimation(joinDialog, "digitalizationProgress", sequentialGroup);
                digitAnim->setStartValue(0.0);
                digitAnim->setEndValue(1.0);
                digitAnim->setEasingCurve(QEasingCurve::InOutQuad);
                digitAnim->setDuration(m_duration * 1.5);

                sequentialGroup->addAnimation(digitAnim); // 2. Animacja digitalizacji

                connect(sequentialGroup, &QSequentialAnimationGroup::finished, this, [this, joinDialog]() {
                    emit showAnimationFinished();
                    joinDialog->setRefreshTimerInterval(100);
                     qDebug() << "LOG: Sekwencja animacji DigitalMaterialization zakończona (JoinWavelengthDialog)";
                });
            }

            // Jeśli dialog nie jest specjalnym typem, uruchom tylko animację wjazdu/pojawienia
            if (!isDialogAnimated) {
                 qDebug() << "LOG: Uruchamiam tylko animację wjazdu/pojawienia dla DigitalMaterialization";
                // Połącz koniec grupy równoległej (bo nie ma sekwencji)
                connect(showParallelGroup, &QParallelAnimationGroup::finished, this, [this]() {
                    emit showAnimationFinished();
                });
                showParallelGroup->start(QAbstractAnimation::DeleteWhenStopped);
                finalAnimation = showParallelGroup; // Ustaw finalAnimation na grupę równoległą
            } else {
                 qDebug() << "LOG: Uruchamiam sekwencję animacji DigitalMaterialization";
                sequentialGroup->start(QAbstractAnimation::DeleteWhenStopped); // Uruchom całą sekwencję
                finalAnimation = sequentialGroup; // Ustaw finalAnimation na grupę sekwencyjną
            }
            // --- KONIEC ZMIANY 2 ---

            break; // Koniec przypadku DigitalMaterialization
        } // Koniec case DigitalMaterialization
    } // Koniec switch

    // --- USUNIĘTO zbędną logikę startowania i łączenia sygnałów dla pojedynczych animacji ---
    // Ta logika jest teraz obsługiwana wewnątrz case DigitalMaterialization
    // lub pozostaje bez zmian dla innych typów animacji (które używają 'animation')

    // Jeśli 'animation' zostało utworzone (dla innych typów niż DigitalMaterialization)
    if (animation && !finalAnimation) {
        animation->setDuration(m_duration);
        connect(animation, &QAbstractAnimation::finished, this, &AnimatedDialog::showAnimationFinished);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        finalAnimation = animation;
    }

    // Jeśli z jakiegoś powodu żadna animacja nie została utworzona/uruchomiona
    if (!finalAnimation) {
        qDebug() << "OSTRZEŻENIE: Nie utworzono/uruchomiono animacji w animateShow()";
        emit showAnimationFinished(); // Emituj sygnał od razu
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

        // W animateClose() zmodyfikuj przypadek DigitalMaterialization:
case DigitalMaterialization: {
    // Animacja wyjazdu do góry
    QRect endGeometry = this->geometry();
    endGeometry.moveTop(endGeometry.top() - 100);

    animation = new QPropertyAnimation(this, "geometry");
    animation->setStartValue(this->geometry());
    animation->setEndValue(endGeometry);
    animation->setEasingCurve(QEasingCurve::InQuint);

    bool isDialogAnimated = false;

    // Obsługa WavelengthDialog
    if (auto digitalDialog = qobject_cast<WavelengthDialog*>(this)) {
        isDialogAnimated = true;

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
    // Obsługa JoinWavelengthDialog
    else if (auto joinDialog = qobject_cast<JoinWavelengthDialog*>(this)) {
        isDialogAnimated = true;

        // Animacja digitalizacji w odwrotną stronę
        QPropertyAnimation* digitAnim = new QPropertyAnimation(joinDialog, "digitalizationProgress");
        digitAnim->setStartValue(joinDialog->digitalizationProgress());
        digitAnim->setEndValue(0.0);
        digitAnim->setDuration(m_duration);

        // Końcowy efekt glitch
        QPropertyAnimation* glitchAnim = new QPropertyAnimation(joinDialog, "glitchIntensity");
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

    // Przypadek awaryjny, gdy nie można rzutować na żaden z typów dialogów
    if (!isDialogAnimated) {
        connect(animation, &QPropertyAnimation::finished, this, [this]() {
            QDialog::close();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        finalAnimation = animation;
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