#include "animated_dialog.h"
#include <QScreen>

#include "../../ui/dialogs/join_wavelength_dialog.h"
#include "../../ui/dialogs/wavelength_dialog.h"
#include "../navigation/navbar.h"
#include "../widgets/overlay_widget.h"

// Implementacja AnimatedDialog
AnimatedDialog::AnimatedDialog(QWidget *parent, const AnimationType type)
    : QDialog(parent),
      animation_type_(type),
      duration_(300),
      closing_(false),
      overlay_(nullptr)
{
    // Niezbędne dla animacji
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

AnimatedDialog::~AnimatedDialog()
{
    // Upewniamy się, że overlay zostanie usunięty
    if (overlay_) {
        overlay_->deleteLater();
    }
}

void AnimatedDialog::showEvent(QShowEvent *event)
{
    QWidget *parent_widget = parentWidget();
    if (!parent_widget) {
        parent_widget = QApplication::activeWindow();
    }

    if (parent_widget) {
        if (!overlay_) {
            overlay_ = new OverlayWidget(parent_widget);
        }

        const auto overlay_animation = new QPropertyAnimation(overlay_, "opacity");
        overlay_animation->setDuration(200);
        overlay_animation->setStartValue(0.0);
        overlay_animation->setEndValue(1.0);
        overlay_animation->setEasingCurve(QEasingCurve::InOutQuad);

        overlay_->UpdateGeometry(parent_widget->rect());
        overlay_->show();
        overlay_animation->start(QAbstractAnimation::DeleteWhenStopped);

        raise();
    }

    event->accept();
    AnimateShow();
}

void AnimatedDialog::closeEvent(QCloseEvent *event)
{
    if (!closing_) {
        event->ignore();
        closing_ = true;

        if (overlay_) {
            const auto overlay_animation = new QPropertyAnimation(overlay_, "opacity");
            overlay_animation->setDuration(200);
            overlay_animation->setStartValue(overlay_->GetOpacity());
            overlay_animation->setEndValue(0.0);
            overlay_animation->setEasingCurve(QEasingCurve::InOutQuad);

            connect(overlay_animation, &QPropertyAnimation::finished, this, [this]() {
                if (overlay_) {
                    overlay_->deleteLater();
                    overlay_ = nullptr;
                }
            });

            overlay_animation->start(QAbstractAnimation::DeleteWhenStopped);
        }

        AnimateClose();
    } else {
        event->accept();
    }
}


void AnimatedDialog::AnimateShow()
{
    const QAbstractAnimation* final_animation = nullptr;

    // Przechowujemy początkową geometrię
    const QRect geometry = this->geometry();

    switch (animation_type_) {
        // Zmodyfikuj przypadek DigitalMaterialization w funkcji animateShow()
        case kDigitalMaterialization: {
            // --- ZMIANA: Połączenie animacji pozycji i przezroczystości ---
            // Ustaw początkową pozycję i przezroczystość
            const int start_y = geometry.y() - 100; // Zmniejszono nieco dystans początkowy dla subtelniejszego efektu
            const int end_y = geometry.y();
            this->move(geometry.x(), start_y);
            this->setWindowOpacity(0.0); // Start fully transparent

            // 1a. Animacja pozycji (wjazd z góry)
            const auto position_animation = new QPropertyAnimation(this, "pos");
            position_animation->setStartValue(QPoint(geometry.x(), start_y));
            position_animation->setEndValue(QPoint(geometry.x(), end_y));
            position_animation->setEasingCurve(QEasingCurve::OutCubic); // Nieco płynniejsza krzywa niż OutQuad
            position_animation->setDuration(duration_); // Czas trwania animacji pozycji

            // 1b. Animacja przezroczystości (płynne pojawienie)
            const auto opacity_animation = new QPropertyAnimation(this, "windowOpacity");
            opacity_animation->setStartValue(0.0);
            opacity_animation->setEndValue(1.0);
            opacity_animation->setEasingCurve(QEasingCurve::OutCubic); // Ta sama krzywa dla spójności
            opacity_animation->setDuration(duration_); // Ten sam czas trwania

            // Grupa równoległa dla jednoczesnego wjazdu i pojawienia się
            const auto show_parallel_group = new QParallelAnimationGroup(this);
            show_parallel_group->addAnimation(position_animation);
            show_parallel_group->addAnimation(opacity_animation);
            // --- KONIEC ZMIANY 1 ---

            // --- ZMIANA 2: Użycie QSequentialAnimationGroup z grupą równoległą ---
            const auto sequential_group = new QSequentialAnimationGroup(this);
            sequential_group->addAnimation(show_parallel_group); // 1. Animacja wjazdu i pojawienia się (równoległa)

            // Sprawdzamy typ dialogu i dodajemy animację digitalizacji *po* animacji wjazdu/pojawienia
            bool is_dialog_animated = false;
            QPropertyAnimation* digital_animation = nullptr;

            // Obsługa WavelengthDialog
            if (auto digital_dialog = qobject_cast<WavelengthDialog*>(this)) {
                is_dialog_animated = true;
                digital_dialog->StartRefreshTimer();

                digital_animation = new QPropertyAnimation(digital_dialog, "digitalizationProgress", sequential_group);
                digital_animation->setStartValue(0.0);
                digital_animation->setEndValue(1.0);
                digital_animation->setEasingCurve(QEasingCurve::InOutQuad);
                digital_animation->setDuration(duration_ * 1.5);

                sequential_group->addAnimation(digital_animation); // 2. Animacja digitalizacji

                connect(sequential_group, &QSequentialAnimationGroup::finished, this, [this, digital_dialog]() {
                    emit showAnimationFinished();
                    digital_dialog->SetRefreshTimerInterval(100);
                    qDebug() << "LOG: Sekwencja animacji DigitalMaterialization zakończona (WavelengthDialog)";
                });
            }
            // Obsługa JoinWavelengthDialog
            else if (auto join_dialog = qobject_cast<JoinWavelengthDialog*>(this)) {
                is_dialog_animated = true;
                join_dialog->StartRefreshTimer();

                digital_animation = new QPropertyAnimation(join_dialog, "digitalizationProgress", sequential_group);
                digital_animation->setStartValue(0.0);
                digital_animation->setEndValue(1.0);
                digital_animation->setEasingCurve(QEasingCurve::InOutQuad);
                digital_animation->setDuration(duration_ * 1.5);

                sequential_group->addAnimation(digital_animation); // 2. Animacja digitalizacji

                connect(sequential_group, &QSequentialAnimationGroup::finished, this, [this, join_dialog]() {
                    emit showAnimationFinished();
                    join_dialog->SetRefreshTimerInterval(100);
                     qDebug() << "LOG: Sekwencja animacji DigitalMaterialization zakończona (JoinWavelengthDialog)";
                });
            }

            // Jeśli dialog nie jest specjalnym typem, uruchom tylko animację wjazdu/pojawienia
            if (!is_dialog_animated) {
                 qDebug() << "LOG: Uruchamiam tylko animację wjazdu/pojawienia dla DigitalMaterialization";
                // Połącz koniec grupy równoległej (bo nie ma sekwencji)
                connect(show_parallel_group, &QParallelAnimationGroup::finished, this, [this]() {
                    emit showAnimationFinished();
                });
                show_parallel_group->start(QAbstractAnimation::DeleteWhenStopped);
                final_animation = show_parallel_group; // Ustaw finalAnimation na grupę równoległą
            } else {
                 qDebug() << "LOG: Uruchamiam sekwencję animacji DigitalMaterialization";
                sequential_group->start(QAbstractAnimation::DeleteWhenStopped); // Uruchom całą sekwencję
                final_animation = sequential_group; // Ustaw finalAnimation na grupę sekwencyjną
            }
            // --- KONIEC ZMIANY 2 ---

            break; // Koniec przypadku DigitalMaterialization
        } // Koniec case DigitalMaterialization
    } // Koniec switch

    // --- USUNIĘTO zbędną logikę startowania i łączenia sygnałów dla pojedynczych animacji ---
    // Ta logika jest teraz obsługiwana wewnątrz case DigitalMaterialization
    // lub pozostaje bez zmian dla innych typów animacji (które używają 'animation')


    // Jeśli z jakiegoś powodu żadna animacja nie została utworzona/uruchomiona
    if (!final_animation) {
        qDebug() << "OSTRZEŻENIE: Nie utworzono/uruchomiono animacji w animateShow()";
        emit showAnimationFinished(); // Emituj sygnał od razu
    }
}

void AnimatedDialog::AnimateClose()
{
    QPropertyAnimation *animation = nullptr;

    // Przechowujemy końcową geometrię (poza ekranem)
    QRect geometry = this->geometry();

    switch (animation_type_) {
        case kSlideFromBottom: {
            // Przesuwamy poza ekran na dole
            geometry.moveTop(QGuiApplication::screenAt(geometry.center())->geometry().bottom());

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(this->geometry());
            animation->setEndValue(geometry);
            animation->setEasingCurve(QEasingCurve::InQuint);
            break;
        }

        case kFadeIn: {
            // Efekt przezroczystości powinien już być ustawiony z animateShow()
            auto effect = qobject_cast<QGraphicsOpacityEffect*>(this->graphicsEffect());
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



        // W animateClose() zmodyfikuj przypadek DigitalMaterialization:
case kDigitalMaterialization: {
    // Animacja wyjazdu do góry
    QRect end_geometry = this->geometry();
    end_geometry.moveTop(end_geometry.top() - 100);

    animation = new QPropertyAnimation(this, "geometry");
    animation->setStartValue(this->geometry());
    animation->setEndValue(end_geometry);
    animation->setEasingCurve(QEasingCurve::InQuint);

    bool is_dialog_animated = false;

    // Obsługa WavelengthDialog
    if (const auto digitalDialog = qobject_cast<WavelengthDialog*>(this)) {
        is_dialog_animated = true;

        // Animacja digitalizacji w odwrotną stronę
        const auto digital_animation = new QPropertyAnimation(digitalDialog, "digitalizationProgress");
        digital_animation->setStartValue(digitalDialog->GetDigitalizationProgress());
        digital_animation->setEndValue(0.0);
        digital_animation->setDuration(duration_);

        // Końcowy efekt glitch
        const auto glitch_animation = new QPropertyAnimation(digitalDialog, "glitchIntensity");
        glitch_animation->setStartValue(0.0);
        glitch_animation->setEndValue(0.0);
        glitch_animation->setKeyValueAt(0.3, 0.8); // Intensywny glitch w trakcie zamykania
        glitch_animation->setDuration(duration_);

        // Grupa animacji
        const auto group = new QParallelAnimationGroup();
        group->addAnimation(animation);
        group->addAnimation(digital_animation);
        group->addAnimation(glitch_animation);

        connect(group, &QAbstractAnimation::finished, this, [this]() {
            QDialog::close();
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }
    // Obsługa JoinWavelengthDialog
    else if (const auto join_dialog = qobject_cast<JoinWavelengthDialog*>(this)) {
        is_dialog_animated = true;

        // Animacja digitalizacji w odwrotną stronę
        const auto digital_animation = new QPropertyAnimation(join_dialog, "digitalizationProgress");
        digital_animation->setStartValue(join_dialog->GetDigitalizationProgress());
        digital_animation->setEndValue(0.0);
        digital_animation->setDuration(duration_);

        // Końcowy efekt glitch
        const auto glitch_animation = new QPropertyAnimation(join_dialog, "glitchIntensity");
        glitch_animation->setStartValue(0.0);
        glitch_animation->setEndValue(0.0);
        glitch_animation->setKeyValueAt(0.3, 0.8); // Intensywny glitch w trakcie zamykania
        glitch_animation->setDuration(duration_);

        // Grupa animacji
        const auto group = new QParallelAnimationGroup();
        group->addAnimation(animation);
        group->addAnimation(digital_animation);
        group->addAnimation(glitch_animation);

        connect(group, &QAbstractAnimation::finished, this, [this]() {
            close();
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    // Przypadek awaryjny, gdy nie można rzutować na żaden z typów dialogów
    if (!is_dialog_animated) {
        connect(animation, &QPropertyAnimation::finished, this, [this]() {
            close();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
    break;
}

    }

    if (animation) {
        animation->setDuration(duration_);
        connect(animation, &QPropertyAnimation::finished, this, [this]() {
            close();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}