#include "animated_dialog.h"

#include <QApplication>
#include <QScreen>

#include "../../ui/dialogs/join_wavelength_dialog.h"
#include "../widgets/overlay_widget.h"

AnimatedDialog::AnimatedDialog(QWidget *parent, const AnimationType type)
    : QDialog(parent),
      animation_type_(type),
      duration_(300),
      closing_(false),
      overlay_(nullptr) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
}

AnimatedDialog::~AnimatedDialog() {
    if (overlay_) {
        overlay_->deleteLater();
    }
}

void AnimatedDialog::showEvent(QShowEvent *event) {
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

void AnimatedDialog::closeEvent(QCloseEvent *event) {
    if (!closing_) {
        event->ignore();
        closing_ = true;

        if (overlay_) {
            const auto overlay_animation = new QPropertyAnimation(overlay_, "opacity");
            overlay_animation->setDuration(200);
            overlay_animation->setStartValue(overlay_->GetOpacity());
            overlay_animation->setEndValue(0.0);
            overlay_animation->setEasingCurve(QEasingCurve::InOutQuad);

            connect(overlay_animation, &QPropertyAnimation::finished, this, [this] {
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


void AnimatedDialog::AnimateShow() {
    const QAbstractAnimation *final_animation = nullptr;

    const QRect geometry = this->geometry();

    if (animation_type_ == kDigitalMaterialization) {
        const int start_y = geometry.y() - 100;
        const int end_y = geometry.y();
        this->move(geometry.x(), start_y);
        this->setWindowOpacity(0.0);

        // 1a. animation of the position (entry from above)
        const auto position_animation = new QPropertyAnimation(this, "pos");
        position_animation->setStartValue(QPoint(geometry.x(), start_y));
        position_animation->setEndValue(QPoint(geometry.x(), end_y));
        position_animation->setEasingCurve(QEasingCurve::OutCubic);
        position_animation->setDuration(duration_);

        // 1b. opacity animation (smooth appearance)
        const auto opacity_animation = new QPropertyAnimation(this, "windowOpacity");
        opacity_animation->setStartValue(0.0);
        opacity_animation->setEndValue(1.0);
        opacity_animation->setEasingCurve(QEasingCurve::OutCubic);
        opacity_animation->setDuration(duration_);

        const auto show_parallel_group = new QParallelAnimationGroup(this);
        show_parallel_group->addAnimation(position_animation);
        show_parallel_group->addAnimation(opacity_animation);

        const auto sequential_group = new QSequentialAnimationGroup(this);
        sequential_group->addAnimation(show_parallel_group); // 1. animation of entry and appearance (parallel)

        bool is_dialog_animated = false;
        QPropertyAnimation *digital_animation = nullptr;

        if (auto digital_dialog = qobject_cast<CreateWavelengthDialog *>(this)) {
            is_dialog_animated = true;
            digital_dialog->StartRefreshTimer();

            digital_animation = new QPropertyAnimation(digital_dialog, "digitalizationProgress", sequential_group);
            digital_animation->setStartValue(0.0);
            digital_animation->setEndValue(1.0);
            digital_animation->setEasingCurve(QEasingCurve::InOutQuad);
            digital_animation->setDuration(duration_ * 1.5);

            sequential_group->addAnimation(digital_animation); // 2. animation of digitization

            connect(sequential_group, &QSequentialAnimationGroup::finished, this, [this, digital_dialog] {
                emit showAnimationFinished();
                digital_dialog->SetRefreshTimerInterval(100);
            });
        } else if (auto join_dialog = qobject_cast<JoinWavelengthDialog *>(this)) {
            is_dialog_animated = true;
            join_dialog->StartRefreshTimer();

            digital_animation = new QPropertyAnimation(join_dialog, "digitalizationProgress", sequential_group);
            digital_animation->setStartValue(0.0);
            digital_animation->setEndValue(1.0);
            digital_animation->setEasingCurve(QEasingCurve::InOutQuad);
            digital_animation->setDuration(duration_ * 1.5);

            sequential_group->addAnimation(digital_animation); // 2. animation of digitization

            connect(sequential_group, &QSequentialAnimationGroup::finished, this, [this, join_dialog] {
                emit showAnimationFinished();
                join_dialog->SetRefreshTimerInterval(100);
            });
        }

        if (!is_dialog_animated) {
            connect(show_parallel_group, &QParallelAnimationGroup::finished, this, [this] {
                emit showAnimationFinished();
            });
            show_parallel_group->start(QAbstractAnimation::DeleteWhenStopped);
            final_animation = show_parallel_group;
        } else {
            sequential_group->start(QAbstractAnimation::DeleteWhenStopped);
            final_animation = sequential_group;
        }
    }

    if (!final_animation) {
        qDebug() << "[ANIMATED DIALOG] WARNING: Failed to create/start animation in animateShow().";
        emit showAnimationFinished();
    }
}

void AnimatedDialog::AnimateClose() {
    QPropertyAnimation *animation = nullptr;

    QRect geometry = this->geometry();

    switch (animation_type_) {
        case kSlideFromBottom: {
            geometry.moveTop(QGuiApplication::screenAt(geometry.center())->geometry().bottom());

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(this->geometry());
            animation->setEndValue(geometry);
            animation->setEasingCurve(QEasingCurve::InQuint);
            break;
        }

        case kDigitalMaterialization: {
            QRect end_geometry = this->geometry();
            end_geometry.moveTop(end_geometry.top() - 100);

            animation = new QPropertyAnimation(this, "geometry");
            animation->setStartValue(this->geometry());
            animation->setEndValue(end_geometry);
            animation->setEasingCurve(QEasingCurve::InQuint);

            bool is_dialog_animated = false;

            if (const auto digitalDialog = qobject_cast<CreateWavelengthDialog *>(this)) {
                is_dialog_animated = true;

                const auto digital_animation = new QPropertyAnimation(digitalDialog, "digitalizationProgress");
                digital_animation->setStartValue(digitalDialog->GetDigitalizationProgress());
                digital_animation->setEndValue(0.0);
                digital_animation->setDuration(duration_);

                const auto glitch_animation = new QPropertyAnimation(digitalDialog, "glitchIntensity");
                glitch_animation->setStartValue(0.0);
                glitch_animation->setEndValue(0.0);
                glitch_animation->setKeyValueAt(0.3, 0.8);
                glitch_animation->setDuration(duration_);

                const auto group = new QParallelAnimationGroup();
                group->addAnimation(animation);
                group->addAnimation(digital_animation);
                group->addAnimation(glitch_animation);

                connect(group, &QAbstractAnimation::finished, this, [this] {
                    close();
                });

                group->start(QAbstractAnimation::DeleteWhenStopped);
            } else if (const auto join_dialog = qobject_cast<JoinWavelengthDialog *>(this)) {
                is_dialog_animated = true;

                const auto digital_animation = new QPropertyAnimation(join_dialog, "digitalizationProgress");
                digital_animation->setStartValue(join_dialog->GetDigitalizationProgress());
                digital_animation->setEndValue(0.0);
                digital_animation->setDuration(duration_);

                const auto glitch_animation = new QPropertyAnimation(join_dialog, "glitchIntensity");
                glitch_animation->setStartValue(0.0);
                glitch_animation->setEndValue(0.0);
                glitch_animation->setKeyValueAt(0.3, 0.8);
                glitch_animation->setDuration(duration_);

                const auto group = new QParallelAnimationGroup();
                group->addAnimation(animation);
                group->addAnimation(digital_animation);
                group->addAnimation(glitch_animation);

                connect(group, &QAbstractAnimation::finished, this, [this] {
                    close();
                });

                group->start(QAbstractAnimation::DeleteWhenStopped);
            }

            if (!is_dialog_animated) {
                connect(animation, &QPropertyAnimation::finished, this, [this] {
                    close();
                });
                animation->start(QAbstractAnimation::DeleteWhenStopped);
            }
            break;
        }
        default:
            break;
    }

    if (animation) {
        animation->setDuration(duration_);
        connect(animation, &QPropertyAnimation::finished, this, [this] {
            close();
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
