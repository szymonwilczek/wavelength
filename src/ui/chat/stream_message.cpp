#include "stream_message.h"

#include <cmath>
#include <QParallelAnimationGroup>
#include <QScrollBar>
#include <QSequentialAnimationGroup>
#include <QBitmap>
#include <QDateTime>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSvgRenderer>
#include <QTimer>

#include "../../chat/files/attachments/attachment_placeholder.h"
#include "../../chat/files/attachments/auto_scaling_attachment.h"
#include "../files/attachment_viewer.h"
#include "effects/electronic_shutdown_effect.h"
#include "effects/long_text_display_effect.h"
#include "effects/text_display_effect.h"

StreamMessage::StreamMessage(QString content, QString sender, MessageType type, QString message_id,
                             QWidget *parent): QWidget(parent), message_id_(std::move(message_id)),
                                               content_(std::move(content)), sender_(std::move(sender)),
                                               type_(type),
                                               opacity_(0.0), glow_intensity_(0.8),
                                               is_read_(false) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumWidth(400);
    setMaximumWidth(600);
    setMinimumHeight(120);

    main_layout_ = new QVBoxLayout(this);
    main_layout_->setContentsMargins(45, 35, 45, 30);
    main_layout_->setSpacing(10);

    CleanupContent();

    bool has_attachment = content_.contains("placeholder");
    bool is_long_message = clean_content_.length() > 500;

    if (!has_attachment) {
        if (is_long_message) {
            QColor text_color;
            switch (type_) {
                case kTransmitted:
                    text_color = QColor(0, 220, 255);
                    break;
                case kReceived:
                    text_color = QColor(240, 150, 255);
                    break;
                case kSystem:
                    text_color = QColor(255, 200, 0);
                    break;
            }

            auto scroll_area = new QScrollArea(this);
            scroll_area->setObjectName("cyberpunkScrollArea");
            scroll_area->setWidgetResizable(true);
            scroll_area->setFrameShape(QFrame::NoFrame);
            scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

            scroll_area->setStyleSheet("QScrollArea { background: transparent; border: none; }");

            auto long_text_display = new LongTextDisplayEffect(clean_content_, text_color);
            scroll_area->setWidget(long_text_display);

            main_layout_->addWidget(scroll_area);
            scroll_area_ = scroll_area;
            long_text_display_ = long_text_display;

            connect(scroll_area->verticalScrollBar(), &QScrollBar::valueChanged,
                    long_text_display, &LongTextDisplayEffect::SetScrollPosition);

            connect(long_text_display, &LongTextDisplayEffect::contentHeightChanged, this,
                    [scroll_area](const int height) {
                        scroll_area->widget()->setMinimumHeight(height);
                    });

            setMinimumHeight(350);
        } else {
            TextDisplayEffect::TypingSoundType sound_type;
            if (sender_ == "SYSTEM") {
                sound_type = TextDisplayEffect::kSystemSound;
            } else {
                sound_type = TextDisplayEffect::kUserSound;
            }

            text_display_ = new TextDisplayEffect(clean_content_, sound_type, this);
            main_layout_->addWidget(text_display_);
        }
    } else {
        content_label_ = new QLabel(this);
        content_label_->setTextFormat(Qt::RichText);
        content_label_->setWordWrap(true);
        content_label_->setStyleSheet(
            "QLabel { color: white; background-color: transparent; font-size: 10pt; }");
        content_label_->setText(clean_content_);
        main_layout_->addWidget(content_label_);
    }

    auto opacity = new QGraphicsOpacityEffect(this);
    opacity->setOpacity(0.0);
    setGraphicsEffect(opacity);

    next_button_ = new QPushButton(">", this);
    next_button_->setFixedSize(25, 25);
    next_button_->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 200, 255, 0.3);"
        "  color: #00ffff;"
        "  border: 1px solid #00ccff;"
        "  border-radius: 3px;"
        "  font-weight: bold;"
        "  font-size: 12pt;"
        "  padding: 0px 0px 1px 0px;"
        "}"
        "QPushButton:hover { background-color: rgba(0, 200, 255, 0.5); }"
        "QPushButton:pressed { background-color: rgba(0, 200, 255, 0.7); }");
    next_button_->hide();

    prev_button_ = new QPushButton("<", this);
    prev_button_->setFixedSize(25, 25);
    prev_button_->setStyleSheet(next_button_->styleSheet());
    prev_button_->hide();

    mark_read_button = new QPushButton(this);
    mark_read_button->setFixedSize(25, 25);

    auto icon_color = QColor("#00ffcc");
    QSize icon_size(20, 20);

    QIcon check_icon = CreateColoredSvgIcon(":/resources/icons/checkmark.svg", icon_color, icon_size);

    if (!check_icon.isNull()) {
        mark_read_button->setIcon(check_icon);
    } else {
        qWarning() << "[STREAM MESSAGE] Cannot create a colored checkmark.svg icon!";
        mark_read_button->setText("✓");
    }

    mark_read_button->setStyleSheet(
        "QPushButton {"
        "  background-color: rgba(0, 255, 150, 0.3);"
        "  border: 1px solid #00ffcc;"
        "  border-radius: 3px;"
        "}"
        "QPushButton:hover { background-color: rgba(0, 255, 150, 0.5); }"
        "QPushButton:pressed { background-color: rgba(0, 255, 150, 0.7); }");
    mark_read_button->hide();

    connect(mark_read_button, &QPushButton::clicked, this, &StreamMessage::MarkAsRead);

    animation_timer_ = new QTimer(this);
    connect(animation_timer_, &QTimer::timeout, this, &StreamMessage::UpdateAnimation);
    animation_timer_->start(50);


    if (scroll_area_) {
        AdjustScrollAreaStyle();
        QTimer::singleShot(100, this, &StreamMessage::UpdateScrollAreaMaxHeight);
    }
}

void StreamMessage::UpdateContent(const QString &new_content) {
    content_ = new_content;
    CleanupContent();

    if (text_display_) {
        text_display_->SetText(clean_content_);
    } else if (long_text_display_) {
        QMetaObject::invokeMethod(long_text_display_, "SetText", Q_ARG(QString, clean_content_));
    } else if (content_label_) {
        content_label_->setText(clean_content_);
    }
    AdjustSizeToContent();
    update();
}

void StreamMessage::SetOpacity(const qreal opacity) {
    opacity_ = opacity;
    if (const auto effect = qobject_cast<QGraphicsOpacityEffect *>(graphicsEffect())) {
        effect->setOpacity(opacity);
    }
}

void StreamMessage::SetGlowIntensity(const qreal intensity) {
    glow_intensity_ = intensity;
    update();
}


void StreamMessage::SetShutdownProgress(const qreal progress) {
    shutdown_progress_ = progress;
    if (shutdown_effect_) {
        shutdown_effect_->SetProgress(progress);
    }
}

void StreamMessage::AddAttachment(const QString &html) {
    QString type, attachment_id, mime_type, filename;

    if (html.contains("video-placeholder")) {
        type = "video";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else if (html.contains("audio-placeholder")) {
        type = "audio";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else if (html.contains("gif-placeholder")) {
        type = "gif";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else if (html.contains("image-placeholder")) {
        type = "image";
        attachment_id = ExtractAttribute(html, "data-attachment-id");
        mime_type = ExtractAttribute(html, "data-mime-type");
        filename = ExtractAttribute(html, "data-filename");
    } else {
        return;
    }

    if (attachment_widget_) {
        main_layout_->removeWidget(attachment_widget_);
        delete attachment_widget_;
    }

    setMinimumHeight(0);
    setMaximumHeight(QWIDGETSIZE_MAX);
    setMinimumWidth(0);
    setMaximumWidth(QWIDGETSIZE_MAX);

    const auto attachment_widget = new AttachmentPlaceholder(
        filename, type, this);
    attachment_widget->SetAttachmentReference(attachment_id, mime_type);

    attachment_widget_ = attachment_widget;
    main_layout_->addWidget(attachment_widget_);

    attachment_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    CleanupContent();
    if (content_label_) {
        content_label_->setText(clean_content_);
    }

    connect(attachment_widget, &AttachmentPlaceholder::attachmentLoaded,
            this, [this] {
                QTimer::singleShot(200, this, [this] {
                    if (attachment_widget_) {
                        QList<AttachmentViewer *> viewers =
                                attachment_widget_->findChildren<AttachmentViewer *>();

                        if (!viewers.isEmpty()) {
                            QTimer::singleShot(100, this, [this, viewers] {
                                viewers.first()->updateGeometry();
                                AdjustSizeToContent();
                            });
                        } else {
                            AdjustSizeToContent();
                        }
                    }
                });
            });

    QTimer::singleShot(100, this, [this] {
        if (attachment_widget_) {
            const QSize initial_size = attachment_widget_->sizeHint();
            setMinimumSize(qMax(initial_size.width() + 60, 500),
                           initial_size.height() + 100);
        }
    });
}

void StreamMessage::AdjustSizeToContent() {
    main_layout_->invalidate();
    main_layout_->activate();

    const QScreen *screen = QApplication::primaryScreen();
    if (window()) {
        screen = window()->screen();
    }

    const QRect available_geometry = screen->availableGeometry();

    int max_width = available_geometry.width() * 0.8;
    int max_height = available_geometry.height() * 0.35;

    QTimer::singleShot(100, this, [this, max_width, max_height] {
        if (attachment_widget_) {
            QList<AttachmentViewer *> viewers = attachment_widget_->findChildren<AttachmentViewer *>();
            if (!viewers.isEmpty()) {
                AttachmentViewer *viewer = viewers.first();
                viewer->updateGeometry();
                const QSize viewer_size = viewer->sizeHint();

                const int new_width = qMin(
                    qMax(viewer_size.width() +
                         main_layout_->contentsMargins().left() +
                         main_layout_->contentsMargins().right() + 100, 500),
                    max_width);
                const int new_height = qMin(
                    viewer_size.height() +
                    main_layout_->contentsMargins().top() +
                    main_layout_->contentsMargins().bottom() + 80,
                    max_height);

                setMinimumSize(new_width, new_height);
                updateGeometry();

                QList<AutoScalingAttachment *> scalers = viewer->findChildren<AutoScalingAttachment *>();
                if (!scalers.isEmpty()) {
                    const QSize content_max_size(
                        max_width - main_layout_->contentsMargins().left() - main_layout_->contentsMargins().right() +
                        20,
                        max_height - main_layout_->contentsMargins().top() - main_layout_->contentsMargins().bottom() -
                        100
                    );

                    scalers.first()->SetMaxAllowedSize(content_max_size);
                }

                if (parentWidget() && parentWidget()->layout()) {
                    parentWidget()->layout()->invalidate();
                    parentWidget()->layout()->activate();
                    parentWidget()->updateGeometry();
                    parentWidget()->update();
                }
            }
        }

        UpdateLayout();
        update();
    });
}

QSize StreamMessage::sizeHint() const {
    constexpr QSize base_size(550, 180);

    if (scroll_area_) {
        return QSize(550, 300);
    }

    if (attachment_widget_) {
        const QSize attachment_size = attachment_widget_->sizeHint();

        QList<const AttachmentViewer *> viewers =
                attachment_widget_->findChildren<const AttachmentViewer *>();

        if (!viewers.isEmpty()) {
            const QSize viewer_size = viewers.first()->sizeHint();
            const int total_height = viewer_size.height() +
                                     main_layout_->contentsMargins().top() +
                                     main_layout_->contentsMargins().bottom() + 80;
            const int total_width = qMax(viewer_size.width() +
                                         main_layout_->contentsMargins().left() +
                                         main_layout_->contentsMargins().right() + 40, base_size.width());

            return QSize(total_width, total_height);
        }

        const int height = attachment_size.height() +
                           main_layout_->contentsMargins().top() +
                           main_layout_->contentsMargins().bottom() + 70;
        const int width = qMax(attachment_size.width() +
                               main_layout_->contentsMargins().left() +
                               main_layout_->contentsMargins().right() + 30, base_size.width());

        return QSize(width, height);
    }

    return base_size;
}

void StreamMessage::FadeIn() {
    const auto opacity_animation = new QPropertyAnimation(this, "opacity");
    opacity_animation->setDuration(400);
    opacity_animation->setStartValue(0.0);
    opacity_animation->setEndValue(1.0);
    opacity_animation->setEasingCurve(QEasingCurve::OutQuad);

    const auto glow_animation = new QPropertyAnimation(this, "glowIntensity");
    glow_animation->setDuration(600);
    glow_animation->setStartValue(0.9);
    glow_animation->setKeyValueAt(0.4, 0.7);
    glow_animation->setEndValue(0.5);
    glow_animation->setEasingCurve(QEasingCurve::OutQuad);

    auto animation_group = new QParallelAnimationGroup(this);
    animation_group->addAnimation(opacity_animation);
    animation_group->addAnimation(glow_animation);

    connect(animation_group, &QParallelAnimationGroup::finished, this, [this, animation_group] {
        animation_group->deleteLater();

        if (text_display_) {
            QTimer::singleShot(50, text_display_, &TextDisplayEffect::StartReveal);
        }
    });

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(50, this, QOverload<>::of(&StreamMessage::setFocus));
}

void StreamMessage::FadeOut() {
    const auto opacity_animation = new QPropertyAnimation(this, "opacity");
    opacity_animation->setDuration(300);
    opacity_animation->setStartValue(1.0);
    opacity_animation->setEndValue(0.0);
    opacity_animation->setEasingCurve(QEasingCurve::InQuad);

    const auto glow_animation = new QPropertyAnimation(this, "glowIntensity");
    glow_animation->setDuration(300);
    glow_animation->setStartValue(0.8);
    glow_animation->setKeyValueAt(0.3, 0.6);
    glow_animation->setKeyValueAt(0.6, 0.3);
    glow_animation->setEndValue(0.0);

    auto animation_group = new QParallelAnimationGroup(this);
    animation_group->addAnimation(opacity_animation);
    animation_group->addAnimation(glow_animation);

    connect(animation_group, &QParallelAnimationGroup::finished, this, [this, animation_group] {
        animation_group->deleteLater();
        hide();
        emit hidden();
    });

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void StreamMessage::UpdateScrollAreaMaxHeight() const {
    if (!scroll_area_)
        return;

    const QScreen *screen = QApplication::primaryScreen();
    if (window()) {
        screen = window()->screen();
    }
    const QRect available_geometry = screen->availableGeometry();

    const int max_height = available_geometry.height() * 0.5;
    scroll_area_->setMaximumHeight(max_height);

    if (QWidget *content_widget = scroll_area_->widget()) {
        content_widget->setMinimumWidth(scroll_area_->width() - 20);
    }
}

void StreamMessage::StartShutdownAnimation() {
    if (graphicsEffect() && graphicsEffect() != shutdown_effect_) {
        delete graphicsEffect();
    }

    shutdown_effect_ = new ElectronicShutdownEffect(this);
    shutdown_effect_->SetProgress(0.0);
    setGraphicsEffect(shutdown_effect_);

    const auto shutdown_animation = new QPropertyAnimation(this, "shutdownProgress");
    shutdown_animation->setDuration(1200);
    shutdown_animation->setStartValue(0.0);
    shutdown_animation->setEndValue(1.0);
    shutdown_animation->setEasingCurve(QEasingCurve::InQuad);

    connect(shutdown_animation, &QPropertyAnimation::finished, this, [this] {
        hide();
        emit hidden();
    });

    shutdown_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void StreamMessage::ShowNavigationButtons(const bool has_previous, const bool has_next) const {
    next_button_->setVisible(has_next);
    prev_button_->setVisible(has_previous);
    mark_read_button->setVisible(true);

    UpdateLayout();

    next_button_->raise();
    prev_button_->raise();
    mark_read_button->raise();
}

void StreamMessage::MarkAsRead() {
    if (!is_read_) {
        is_read_ = true;

        if (scroll_area_ && long_text_display_) {
            StartLongMessageClosingAnimation();
        } else {
            StartShutdownAnimation();
        }

        emit messageRead();
    }
}

void StreamMessage::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor background_color, border_color, glow_color, text_color;

    switch (type_) {
        case kTransmitted:
            background_color = QColor(0, 20, 40, 180);
            border_color = QColor(0, 200, 255);
            glow_color = QColor(0, 150, 255, 80);
            text_color = QColor(0, 220, 255);
            break;
        case kReceived:
            background_color = QColor(30, 0, 30, 180);
            border_color = QColor(220, 50, 255);
            glow_color = QColor(180, 0, 255, 80);
            text_color = QColor(240, 150, 255);
            break;
        case kSystem:
            background_color = QColor(40, 25, 0, 180);
            border_color = QColor(255, 180, 0);
            glow_color = QColor(255, 150, 0, 80);
            text_color = QColor(255, 200, 0);
            break;
    }

    QPainterPath path;
    int clip_size = 20;
    int notch_size = 10;

    // top edge with notch
    path.moveTo(clip_size, 0);
    path.lineTo(width() - clip_size, 0);

    // right-top corner and right edge with notch
    path.lineTo(width(), clip_size);
    path.lineTo(width(), height() / 2 - notch_size);
    path.lineTo(width() - notch_size, height() / 2);
    path.lineTo(width(), height() / 2 + notch_size);
    path.lineTo(width(), height() - clip_size);

    // right-bottom corner and bottom edge with notch
    path.lineTo(width() - clip_size, height());
    path.lineTo(clip_size, height());

    // left-bottom corner and left edge with notch
    path.lineTo(0, height() - clip_size);
    path.lineTo(0, height() / 2 + notch_size);
    path.lineTo(notch_size, height() / 2);
    path.lineTo(0, height() / 2 - notch_size);
    path.lineTo(0, clip_size);

    path.closeSubpath();

    QLinearGradient background_gradient(0, 0, width(), height());
    background_gradient.setColorAt(0, background_color.lighter(110));
    background_gradient.setColorAt(1, background_color);

    painter.setPen(Qt::NoPen);
    painter.setBrush(background_gradient);
    painter.drawPath(path);

    if (glow_intensity_ > 0.1) {
        painter.setPen(QPen(glow_color, 6 + glow_intensity_ * 6));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    painter.setPen(QPen(border_color, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);

    // decorative lines (horizontal)
    painter.setPen(QPen(border_color.lighter(120), 1, Qt::DashLine));
    painter.drawLine(clip_size, 30, width() - clip_size, 30);
    painter.drawLine(40, height() - 25, width() - 40, height() - 25);

    // decorative lines (vertical)
    painter.setPen(QPen(border_color.lighter(120), 1, Qt::DotLine));
    painter.drawLine(40, 30, 40, height() - 25);
    painter.drawLine(width() - 40, 30, width() - 40, height() - 25);

    int ar_marker_size = 15;
    painter.setPen(QPen(border_color, 1, Qt::SolidLine));

    // left-top
    painter.drawLine(clip_size, 10, clip_size + ar_marker_size, 10);
    painter.drawLine(clip_size, 10, clip_size, 10 + ar_marker_size);

    // right-top
    painter.drawLine(width() - clip_size - ar_marker_size, 10, width() - clip_size, 10);
    painter.drawLine(width() - clip_size, 10, width() - clip_size, 10 + ar_marker_size);

    // header text
    painter.setPen(QPen(text_color, 1));
    painter.setFont(QFont("Consolas", 10, QFont::Bold));
    painter.drawText(QRect(clip_size + 5, 7, width() - 2 * clip_size - 10, 22),
                     Qt::AlignLeft | Qt::AlignVCenter, sender_);

    QDateTime current_time = QDateTime::currentDateTime();
    QString time_string = current_time.toString("HH:mm:ss");

    painter.setFont(QFont("Consolas", 8));
    painter.setPen(QPen(text_color.lighter(120), 1));

    painter.drawText(QRect(width() - 150, 12, 120, 12),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QString("TS: %1").arg(time_string));

    QColor priority_color;
    QString priority_text;

    switch (type_) {
        case kTransmitted:
            priority_text = "OUT";
            priority_color = QColor(0, 220, 255);
            break;
        case kReceived:
            priority_text = "IN";
            priority_color = QColor(255, 50, 240);
            break;
        case kSystem:
            priority_text = "SYS";
            priority_color = QColor(255, 220, 0);
            break;
    }

    QRect priority_rect(width() - 70, height() - 40, 60, 20);
    painter.setPen(QPen(priority_color, 1, Qt::SolidLine));
    painter.setBrush(QBrush(priority_color.darker(600)));
    painter.drawRect(priority_rect);

    painter.setPen(QPen(priority_color, 1));
    painter.setFont(QFont("Consolas", 8, QFont::Bold));
    painter.drawText(priority_rect, Qt::AlignCenter, priority_text);
}

void StreamMessage::resizeEvent(QResizeEvent *event) {
    UpdateLayout();

    if (scroll_area_) {
        if (QWidget *content_widget = scroll_area_->widget()) {
            content_widget->setMinimumWidth(scroll_area_->width() - 30);
        }
    }

    QWidget::resizeEvent(event);
}

void StreamMessage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
        MarkAsRead();
        event->accept();
    } else if (event->key() == Qt::Key_Right && next_button_->isVisible()) {
        next_button_->click();
        event->accept();
    } else if (event->key() == Qt::Key_Left && prev_button_->isVisible()) {
        prev_button_->click();
        event->accept();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void StreamMessage::focusInEvent(QFocusEvent *event) {
    glow_intensity_ += 0.2;
    update();
    QWidget::focusInEvent(event);
}

void StreamMessage::focusOutEvent(QFocusEvent *event) {
    glow_intensity_ -= 0.2;
    update();
    QWidget::focusOutEvent(event);
}

void StreamMessage::UpdateAnimation() {
    const qreal pulse = 0.05 * sin(QDateTime::currentMSecsSinceEpoch() * 0.002);
    SetGlowIntensity(glow_intensity_ + pulse * (!is_read_ ? 1.0 : 0.3));
}

void StreamMessage::AdjustScrollAreaStyle() const {
    if (!scroll_area_) return;

    QString handle_color;
    QColor text_color;

    switch (type_) {
        case kTransmitted:
            handle_color = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #00c8ff, stop:1 #0080ff)";
            text_color = QColor(0, 220, 255);
            break;
        case kReceived:
            handle_color = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ff50dd, stop:1 #b000ff)";
            text_color = QColor(240, 150, 255);
            break;
        case kSystem:
            handle_color = "qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #ffb400, stop:1 #ff8000)";
            text_color = QColor(255, 200, 0);
            break;
    }

    QString style = QString(
                "QScrollArea { background: transparent; border: none; }"
                "QScrollBar:vertical { background: rgba(10, 20, 30, 150); width: 10px; margin: 0; }"
                "QScrollBar::handle:vertical { background: %1; "
                "border-radius: 5px; min-height: 30px; }"
                "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: rgba(0, 20, 40, 100); }")
            .arg(handle_color);

    scroll_area_->setStyleSheet(style);

    if (long_text_display_) {
        long_text_display_->SetTextColor(text_color);
    }
}

QString StreamMessage::ExtractAttribute(const QString &html, const QString &attribute) {
    int attribute_position = html.indexOf(attribute + "='");
    if (attribute_position >= 0) {
        attribute_position += attribute.length() + 2;
        const int endPos = html.indexOf("'", attribute_position);
        if (endPos >= 0) {
            return html.mid(attribute_position, endPos - attribute_position);
        }
    }
    return QString();
}

void StreamMessage::StartLongMessageClosingAnimation() {
    const auto animation_group = new QSequentialAnimationGroup(this);

    // phase 1: Rapid flickering (glitch effect)
    const auto glitch_animation = new QPropertyAnimation(this, "glowIntensity");
    glitch_animation->setDuration(300);
    glitch_animation->setStartValue(0.8);
    glitch_animation->setKeyValueAt(0.2, 1.2);
    glitch_animation->setKeyValueAt(0.4, 0.3);
    glitch_animation->setKeyValueAt(0.6, 1.0);
    glitch_animation->setKeyValueAt(0.8, 0.5);
    glitch_animation->setEndValue(0.9);
    glitch_animation->setEasingCurve(QEasingCurve::OutInQuad);
    animation_group->addAnimation(glitch_animation);

    // phase 2: Quick disappearance of the entire message
    const auto fade_animation = new QPropertyAnimation(this, "opacity");
    fade_animation->setDuration(250); // short time = quick animation
    fade_animation->setStartValue(1.0);
    fade_animation->setEndValue(0.0);
    fade_animation->setEasingCurve(QEasingCurve::OutQuad);
    animation_group->addAnimation(fade_animation);

    connect(animation_group, &QSequentialAnimationGroup::finished, this, [this] {
        hide();
        emit hidden();
    });

    animation_group->start(QAbstractAnimation::DeleteWhenStopped);
}

void StreamMessage::ProcessImageAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "image", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::ProcessGifAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "gif", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::ProcessAudioAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "audio", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::ProcessVideoAttachment(const QString &html) {
    const QString attachment_id = ExtractAttribute(html, "data-attachment-id");
    const QString mime_type = ExtractAttribute(html, "data-mime-type");
    const QString filename = ExtractAttribute(html, "data-filename");

    const auto container = new QWidget(this);
    const auto container_layout = new QVBoxLayout(container);
    container_layout->setContentsMargins(5, 5, 5, 5);

    const auto placeholder_widget = new AttachmentPlaceholder(
        filename, "video", container);
    placeholder_widget->SetAttachmentReference(attachment_id, mime_type);

    container_layout->addWidget(placeholder_widget);

    attachment_widget_ = container;
    main_layout_->addWidget(attachment_widget_);

    setMinimumHeight(150 + placeholder_widget->sizeHint().height());
}

void StreamMessage::CleanupContent() {
    clean_content_ = content_;

    clean_content_.remove(QRegExp("<[^>]*>"));

    if (content_.contains("placeholder")) {
        const int placeholder_start = content_.indexOf("<div class='");
        if (placeholder_start >= 0) {
            const int placeholder_end = content_.indexOf("</div>", placeholder_start);
            if (placeholder_end > 0) {
                const QString before = clean_content_.left(placeholder_start);
                const QString after = clean_content_.mid(placeholder_end + 6);
                clean_content_ = before.trimmed() + " " + after.trimmed();
            }
        }
    }

    clean_content_.replace("&nbsp;", " ");
    clean_content_.replace("&lt;", "<");
    clean_content_.replace("&gt;", ">");
    clean_content_.replace("&amp;", "&");

    if (clean_content_.length() > 300) {
        clean_content_.replace("\n\n", "||PARAGRAPH||");
        clean_content_ = clean_content_.simplified();
        clean_content_.replace("||PARAGRAPH||", "\n\n");

        clean_content_.replace(". ", ".\n");
        clean_content_.replace("? ", "?\n");
        clean_content_.replace("! ", "!\n");
        clean_content_.replace(":\n", ": ");
    } else {
        clean_content_ = clean_content_.simplified();
    }
}

void StreamMessage::UpdateLayout() const {
    if (next_button_->isVisible()) {
        next_button_->move(width() - next_button_->width() - 10, height() / 2 - next_button_->height() / 2);
    }
    if (prev_button_->isVisible()) {
        prev_button_->move(10, height() / 2 - prev_button_->height() / 2);
    }
    if (mark_read_button->isVisible()) {
        mark_read_button->move(width() - mark_read_button->width() - 10, height() - mark_read_button->height() - 10);
    }
}

QIcon StreamMessage::CreateColoredSvgIcon(const QString &svg_path, const QColor &color, const QSize &size) {
    QSvgRenderer renderer;
    if (!renderer.load(svg_path)) {
        qWarning() << "[STREAM MESSAGE] Unable to load SVG:" << svg_path;
        return QIcon();
    }

    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();

    const QBitmap mask = pixmap.createMaskFromColor(Qt::transparent);

    QPixmap colored_pixmap(size);
    colored_pixmap.fill(color);
    colored_pixmap.setMask(mask);

    return QIcon(colored_pixmap);
}
