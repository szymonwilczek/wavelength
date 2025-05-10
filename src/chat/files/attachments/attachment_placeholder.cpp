#include "attachment_placeholder.h"

#include <qfileinfo.h>
#include <QScrollBar>

#include "attachment_data_store.h"
#include "auto_scaling_attachment.h"
#include "../../../app/managers/translation_manager.h"
#include "../image/displayer/image_viewer.h"
#include "../../../ui/files/cyber_attachment_viewer.h"

AttachmentPlaceholder::AttachmentPlaceholder(const QString &filename, const QString &type,
                                             QWidget *parent): QWidget(parent), filename_(filename), is_loaded_(false) {
    translator_ = TranslationManager::GetInstance();
    const auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    QString icon;
    if (type == "image") icon = "🖼️";
    else if (type == "audio") icon = "🎵";
    else if (type == "video") icon = "📹";
    else if (type == "gif") icon = "🎞️";
    else icon = "📎";

    const QFileInfo file_info(filename);
    const QString base_name = file_info.baseName();
    const QString suffix = file_info.completeSuffix();

    constexpr int max_base_name_length = 25;
    QString displayed_name;

    if (base_name.length() > max_base_name_length) {
        displayed_name = base_name.left(max_base_name_length) + "[...]";
    } else {
        displayed_name = base_name;
    }

    if (!suffix.isEmpty()) {
        displayed_name += "." + suffix;
    }

    info_label_ = new QLabel(
        QString("<span style='color:#00ccff;'>%1</span> <span style='color:#aaaaaa; font-size:9pt;'>%2</span>").
        arg(icon, displayed_name), this);
    info_label_->setTextFormat(Qt::RichText);
    layout->addWidget(info_label_);

    load_button_ = new QPushButton(translator_->Translate("Attachments.Redecode", "REDECODE"), this);
    load_button_->setStyleSheet(
        "QPushButton {"
        "  background-color: #002b3d;"
        "  color: #00ffff;"
        "  border: 1px solid #00aaff;"
        "  border-radius: 2px;"
        "  padding: 6px;"
        "  font-family: 'Consolas', monospace;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #003e59;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #005580;"
        "}"
    );
    load_button_->setVisible(false);
    layout->addWidget(load_button_);

    content_container_ = new QWidget(this);
    content_container_->setVisible(false);
    content_layout_ = new QVBoxLayout(content_container_);
    content_layout_->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(content_container_);

    progress_label_ = new QLabel(
        translator_->Translate("Attachments.InitializingDecode", "Initializing decode sequence..."), this);
    progress_label_->setStyleSheet(
        "QLabel {"
        "  color: #00ccff;"
        "  font-family: 'Consolas', monospace;"
        "  font-weight: bold;"
        "}"
    );
    layout->addWidget(progress_label_);

    connect(load_button_, &QPushButton::clicked, this, &AttachmentPlaceholder::onLoadButtonClicked);

    QTimer::singleShot(300, this, &AttachmentPlaceholder::onLoadButtonClicked);
}

void AttachmentPlaceholder::SetContent(QWidget *content) {
    QLayoutItem *item;
    while ((item = content_layout_->takeAt(0)) != nullptr) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }


    content_layout_->addWidget(content);
    content_container_->setVisible(true);
    load_button_->setVisible(false);
    progress_label_->setVisible(false);
    is_loaded_ = true;

    content->setMinimumSize(0, 0);
    content->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    content_container_->setMinimumSize(0, 0);
    content_container_->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setMinimumSize(0, 0);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    content_container_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    content_layout_->activate();
    layout()->activate();
    updateGeometry();

    QTimer::singleShot(50, this, [this, content]() {
        if (const auto viewer = qobject_cast<CyberAttachmentViewer *>(content)) {
            viewer->UpdateContentLayout();
        } else {
            content->updateGeometry();
        }
        updateGeometry();

        // notify parent widget about geometry change
        if (parentWidget()) {
            parentWidget()->updateGeometry();
            if (parentWidget()->layout()) parentWidget()->layout()->activate();
            QEvent event(QEvent::LayoutRequest);
            QApplication::sendEvent(parentWidget(), &event);
        }

        QTimer::singleShot(50, this, &AttachmentPlaceholder::NotifyLoaded);
    });
}

QSize AttachmentPlaceholder::sizeHint() const {
    constexpr QSize base_size(400, 100);

    const QWidget *content = nullptr;
    if (content_container_ && content_container_->isVisible() && content_layout_->count() > 0) {
        content = content_layout_->itemAt(0)->widget();
    }

    if (content) {
        const QSize content_size = content->sizeHint();
        if (content_size.isValid() && content_size.width() > 0 && content_size.height() > 0) {
            const int total_height = content_size.height() +
                                     info_label_->sizeHint().height() +
                                     (progress_label_->isVisible() ? progress_label_->sizeHint().height() : 0) +
                                     (load_button_->isVisible() ? load_button_->sizeHint().height() : 0) + 20;

            const int total_width = qMax(content_size.width(), base_size.width());

            const QSize result(total_width, total_height);
            return result;
        }
    }

    return base_size;
}

void AttachmentPlaceholder::SetAttachmentReference(const QString &attachment_id, const QString &mime_type) {
    attachment_id_ = attachment_id;
    mime_type_ = mime_type;
    has_reference_ = true;
}

void AttachmentPlaceholder::SetBase64Data(const QString &base64_data, const QString &mime_type) {
    base64_data_ = base64_data;
    mime_type_ = mime_type;
}

void AttachmentPlaceholder::SetLoading(const bool loading) const {
    if (loading) {
        load_button_->setEnabled(false);
        load_button_->setText(translator_->Translate("Attachments.Decoding", "Decoding..."));
        progress_label_->setText(
            translator_->Translate("Attachments.ObtainingDecryptedData", "Obtaining encrypted data..."));
        progress_label_->setStyleSheet(
            "QLabel {"
            "  color: #00ccff;"
            "  font-family: 'Consolas', monospace;"
            "  font-weight: bold;"
            "}"
        );
        progress_label_->setVisible(true);
    } else {
        load_button_->setEnabled(true);
        load_button_->setText(translator_->Translate("Attachments.InitDecode", "INIT DECOODE SEQUENCE"));
        progress_label_->setVisible(false);
    }
}

void AttachmentPlaceholder::onLoadButtonClicked() {
    if (is_loaded_) return;

    SetLoading(true);

    if (has_reference_) {
        AttachmentQueueManager::GetInstance()->AddTask([this]() {
            const QString base64_data = AttachmentDataStore::GetInstance()->GetAttachmentData(attachment_id_);

            if (base64_data.isEmpty()) {
                QMetaObject::invokeMethod(this, "SetError",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString,
                                                translator_->Translate("Attachments.DataNotFound",
                                                    "⚠️ There was an error trying to find attachment data")));
                return;
            }

            const QByteArray data = QByteArray::fromBase64(base64_data.toUtf8());

            if (data.isEmpty()) {
                QMetaObject::invokeMethod(this, "SetError",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString,
                                                translator_->Translate("Attachments.DecodeError",
                                                    "⚠️ There was an error trying to decode attachment data")));
                return;
            }

            if (mime_type_.startsWith("image/")) {
                if (mime_type_ == "image/gif") {
                    QMetaObject::invokeMethod(this, "ShowCyberGif",
                                              Qt::QueuedConnection,
                                              Q_ARG(QByteArray, data));
                } else {
                    QMetaObject::invokeMethod(this, "ShowCyberImage",
                                              Qt::QueuedConnection,
                                              Q_ARG(QByteArray, data));
                }
            } else if (mime_type_.startsWith("audio/")) {
                QMetaObject::invokeMethod(this, "ShowCyberAudio",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, data));
            } else if (mime_type_.startsWith("video/")) {
                QMetaObject::invokeMethod(this, "ShowCyberVideo",
                                          Qt::QueuedConnection,
                                          Q_ARG(QByteArray, data));
            }
        });
    } else {
        AttachmentQueueManager::GetInstance()->AddTask([this]() {
            try {
                const QByteArray data = QByteArray::fromBase64(base64_data_.toUtf8());

                if (data.isEmpty()) {
                    QMetaObject::invokeMethod(this, "SetError",
                                              Qt::QueuedConnection,
                                              Q_ARG(QString,
                                                    translator_->Translate("Attachments.DecodeError",
                                                        "⚠️ There was an error trying to decode attachment data")));
                }
            } catch (const std::exception &e) {
                QMetaObject::invokeMethod(this, "SetError",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString,
                                                QString("⚠️ %1: %2").arg(translator_->Translate(
                                                    "Attachments.ProcessingError",
                                                    "There was an error trying to process the attachment")).arg(e.what()
                                                )));
            }
        });
    }
}

void AttachmentPlaceholder::SetError(const QString &error_msg) {
    load_button_->setEnabled(true);
    load_button_->setText(translator_->Translate("Attachments.Redecode", "REDECODE"));
    load_button_->setVisible(true);
    progress_label_->setText("<span style='color:#ff3333;'>⚠️ ERROR: " + error_msg + "</span>");
    progress_label_->setVisible(true);
    is_loaded_ = false;
}

void AttachmentPlaceholder::ShowFullSizeDialog(const QByteArray &data, const bool is_gif) {
    QWidget *parentWindow = window();
    auto full_size_dialog = new QDialog(parentWindow,
                                        Qt::Window | Qt::FramelessWindowHint | Qt::WindowMaximizeButtonHint |
                                        Qt::WindowCloseButtonHint);

    full_size_dialog->setWindowTitle(translator_->Translate("Attachments.FullSizeDialog", "Full Size"));
    full_size_dialog->setModal(false);
    full_size_dialog->setAttribute(Qt::WA_DeleteOnClose);

    const auto layout = new QVBoxLayout(full_size_dialog);
    layout->setContentsMargins(5, 5, 5, 5);

    auto scroll_area = new QScrollArea(full_size_dialog);
    scroll_area->setWidgetResizable(true);
    scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll_area->setStyleSheet("QScrollArea { background-color: #001018; border: none; }");

    QWidget *content_widget = nullptr;

    auto show_with_size_check = [this, full_size_dialog, scroll_area](QWidget *contentWgt, const QSize size) {
        if (size.isValid()) {
            AdjustAndShowDialog(full_size_dialog, scroll_area, contentWgt, size);
        } else {
            AdjustAndShowDialog(full_size_dialog, scroll_area, contentWgt, QSize(400, 300));
        }
    };

    if (is_gif) {
        InlineGifPlayer *full_gif = nullptr;
        full_gif = new InlineGifPlayer(data, scroll_area);
        content_widget = full_gif;

        connect(full_gif, &InlineGifPlayer::gifLoaded, this, [=]() mutable {
            QTimer::singleShot(0, this, [=]() mutable {
                show_with_size_check(full_gif, full_gif->sizeHint());
                if (full_gif) {
                    full_gif->StartPlayback();
                }
            });
        });
    } else {
        const auto full_image = new InlineImageViewer(data, scroll_area);
        content_widget = full_image;

        connect(full_image, &InlineImageViewer::imageLoaded, this, [=]() {
            QTimer::singleShot(0, this, [=] {
                show_with_size_check(full_image, full_image->sizeHint());
            });
        });
        connect(full_image, &InlineImageViewer::imageInfoReady, this, [=](const int w, const int h, bool) {
            if (!full_size_dialog->isVisible()) {
                QTimer::singleShot(0, this, [=]() {
                    const QSize currentHint = full_image->sizeHint();
                    if (currentHint.isValid()) {
                        show_with_size_check(full_image, currentHint);
                    } else {
                        show_with_size_check(full_image, QSize(w, h));
                    }
                });
            }
        });
    }

    scroll_area->setWidget(content_widget);
    layout->addWidget(scroll_area, 1);

    const auto closeButton = new QPushButton(translator_->Translate("Global.Close", "Close"), full_size_dialog);
    closeButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #002b3d;"
        "  color: #00ffff;"
        "  border: 1px solid #00aaff;"
        "  border-radius: 2px;"
        "  padding: 6px;"
        "  font-family: 'Consolas', monospace;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #003e59;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #005580;"
        "}"
    );
    layout->addWidget(closeButton, 0, Qt::AlignHCenter);
    connect(closeButton, &QPushButton::clicked, full_size_dialog, &QDialog::accept);

    full_size_dialog->setStyleSheet(
        "QDialog {"
        "  background-color: #001822;"
        "  border: 2px solid #00aaff;"
        "}"
    );
}

void AttachmentPlaceholder::AdjustAndShowDialog(QDialog *dialog, const QScrollArea *scroll_area,
                                                QWidget *content_widget,
                                                const QSize original_content_size) const {
    if (!dialog || !content_widget || !original_content_size.isValid()) {
        qDebug() << "[ATTACHMENT PLACEHOLDER] Invalid arguments or size.";
        if (dialog) dialog->deleteLater();
        return;
    }

    // 1. download the screen geometry
    const QScreen *screen = nullptr;
    if (window() && window()->windowHandle()) {
        screen = window()->windowHandle()->screen();
    }
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect available_geometry = screen ? screen->availableGeometry() : QRect(0, 0, 1024, 768);

    // 2. margins of the dialog box
    constexpr int margin = 50;

    // 3. set the content size to original
    content_widget->setFixedSize(original_content_size);

    // 4. calculate preferred dialog size based on original content size
    const auto close_button = dialog->findChild<QPushButton *>();
    const int button_height = close_button ? close_button->sizeHint().height() : 30;
    const int vertical_margins = dialog->layout()->contentsMargins().top() + dialog->layout()->contentsMargins().
                                 bottom() + dialog->layout()->spacing() + button_height;
    const int horizontal_margins = dialog->layout()->contentsMargins().left() + dialog->layout()->contentsMargins().
                                   right();

    QSize preferred_dialog_size = original_content_size;
    preferred_dialog_size.rwidth() += horizontal_margins;
    preferred_dialog_size.rheight() += vertical_margins;

    const QSize max_content_area_size(
        available_geometry.width() - margin * 2 - horizontal_margins - scroll_area->verticalScrollBar()->sizeHint().
        width(),
        available_geometry.height() - margin * 2 - vertical_margins - scroll_area->horizontalScrollBar()->sizeHint().
        height()
    );

    if (original_content_size.width() > max_content_area_size.width()) {
        preferred_dialog_size.rheight() += scroll_area->horizontalScrollBar()->sizeHint().height();
    }
    if (original_content_size.height() > max_content_area_size.height()) {
        preferred_dialog_size.rwidth() += scroll_area->verticalScrollBar()->sizeHint().width();
    }


    // 5. limit the size of the dialogue window to the available screen geometry (with margins)
    QSize final_dialog_size = preferred_dialog_size;
    final_dialog_size.setWidth(qMin(final_dialog_size.width(), available_geometry.width() - margin * 2));
    final_dialog_size.setHeight(qMin(final_dialog_size.height(), available_geometry.height() - margin * 2));

    dialog->resize(final_dialog_size);

    // 6. center the dialogue on the screen
    const int x = available_geometry.left() + (available_geometry.width() - final_dialog_size.width()) / 2;
    const int y = available_geometry.top() + (available_geometry.height() - final_dialog_size.height()) / 2;
    dialog->move(x, y);

    dialog->show();
}

void AttachmentPlaceholder::ShowCyberImage(const QByteArray &data) {
    const auto viewer = new CyberAttachmentViewer(content_container_);
    const auto image_viewer = new InlineImageViewer(data, viewer);
    image_viewer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    const auto scaling_attachment = new AutoScalingAttachment(image_viewer, viewer);

    QSize max_size(400, 300);

    if (parentWidget() && parentWidget()->parentWidget()) {
        const QWidget *stream_message = parentWidget()->parentWidget();
        max_size.setWidth(qMin(max_size.width(), stream_message->width() - 50));
        max_size.setHeight(qMin(max_size.height(), stream_message->height() / 2));
    } else {
        if (const QScreen *screen = QApplication::primaryScreen()) {
            max_size.setWidth(qMin(max_size.width(), screen->availableGeometry().width() / 3));
            max_size.setHeight(qMin(max_size.height(), screen->availableGeometry().height() / 3));
        }
    }
    scaling_attachment->SetMaxAllowedSize(max_size);

    connect(scaling_attachment, &AutoScalingAttachment::clicked, this, [this, data]() {
        ShowFullSizeDialog(data, false); // false -> not a GIF
    });

    viewer->SetContent(scaling_attachment);
    SetContent(viewer);
    SetLoading(false);
}

void AttachmentPlaceholder::ShowCyberGif(const QByteArray &data) {
    const auto viewer = new CyberAttachmentViewer(content_container_);
    const auto gif_player = new InlineGifPlayer(data, viewer);
    const auto scaling_attachment = new AutoScalingAttachment(gif_player, viewer);

    QSize max_size(400, 300);
    if (parentWidget() && parentWidget()->parentWidget()) {
        const QWidget *stream_message = parentWidget()->parentWidget();
        max_size.setWidth(qMin(max_size.width(), stream_message->width() - 50));
        max_size.setHeight(qMin(max_size.height(), stream_message->height() / 2));
    } else {
        if (const QScreen *screen = QApplication::primaryScreen()) {
            max_size.setWidth(qMin(max_size.width(), screen->availableGeometry().width() / 3));
            max_size.setHeight(qMin(max_size.height(), screen->availableGeometry().height() / 3));
        }
    }

    scaling_attachment->SetMaxAllowedSize(max_size);

    connect(scaling_attachment, &AutoScalingAttachment::clicked, this, [this, data]() {
        ShowFullSizeDialog(data, true); // true -> it is GIF
    });

    viewer->SetContent(scaling_attachment);
    SetContent(viewer);
    SetLoading(false);
}

void AttachmentPlaceholder::ShowCyberAudio(const QByteArray &data) {
    const auto viewer = new CyberAttachmentViewer(content_container_);
    const auto audio_player = new InlineAudioPlayer(data, mime_type_, viewer);

    viewer->SetContent(audio_player);

    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        load_button_->setText(translator_->Translate("Attachments.LoadAgain", "Load again"));
        load_button_->setVisible(true);
        content_container_->setVisible(false);
    });

    SetContent(viewer);
    SetLoading(false);
}

void AttachmentPlaceholder::ShowCyberVideo(const QByteArray &data) {
    const auto viewer = new CyberAttachmentViewer(content_container_);
    const auto video_preview = new QWidget(viewer);

    const auto preview_layout = new QVBoxLayout(video_preview);
    preview_layout->setContentsMargins(0, 0, 0, 0);

    const auto thumbnail_label = new QLabel(video_preview);
    thumbnail_label->setFixedSize(480, 270);
    thumbnail_label->setAlignment(Qt::AlignCenter);
    thumbnail_label->setStyleSheet("background-color: #000000; color: white; font-size: 48px;");
    thumbnail_label->setText("▶");
    thumbnail_label->setCursor(Qt::PointingHandCursor);

    GenerateThumbnail(data, thumbnail_label);
    preview_layout->addWidget(thumbnail_label);

    const auto play_button = new QPushButton(
        translator_->Translate("Attachments.PlayVideo", "PLAY VIDEO"),
        video_preview);

    play_button->setStyleSheet(
        "QPushButton { "
        "  background-color: #002b3d; "
        "  color: #00ffff; "
        "  border: 1px solid #00aaff; "
        "  padding: 6px; "
        "  border-radius: 2px; "
        "  font-family: 'Consolas', monospace; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { background-color: #003e59; }"
    );
    preview_layout->addWidget(play_button);

    viewer->SetContent(video_preview);

    auto open_player = [this, data]() {
        video_data_ = data;
        auto video_player_overlay = new VideoPlayerOverlay(data, mime_type_, nullptr);

        video_player_overlay->setStyleSheet(
            "QDialog { "
            "  background-color: #001520; "
            "  border: 2px solid #00aaff; "
            "}"
        );

        video_player_overlay->setAttribute(Qt::WA_DeleteOnClose);

        // disconnecting connections before closing
        QMetaObject::Connection conn = connect(video_player_overlay, &QDialog::finished,
                                               [this, video_player_overlay]() {
                                                   disconnect(video_player_overlay, nullptr, this, nullptr);
                                                   video_data_.clear();
                                               });

        video_player_overlay->show();
    };

    connect(thumbnail_label, &QLabel::linkActivated, this, open_player);
    connect(play_button, &QPushButton::clicked, this, open_player);

    thumbnail_label->installEventFilter(this);
    thumbnail_label_ = thumbnail_label;
    ClickHandler_ = open_player;

    connect(viewer, &CyberAttachmentViewer::viewingFinished, this, [this]() {
        load_button_->setText(translator_->Translate("Attachments.LoadAgain", "Load again"));
        load_button_->setVisible(true);
        content_container_->setVisible(false);
    });

    SetContent(viewer);
    SetLoading(false);
}

void AttachmentPlaceholder::GenerateThumbnail(const QByteArray &video_data, QLabel *thumbnail_label) {
    AttachmentQueueManager::GetInstance()->AddTask([this, video_data, thumbnail_label]() {
        try {
            auto temp_decoder = std::make_shared<VideoDecoder>(video_data, nullptr);

            connect(temp_decoder.get(), &VideoDecoder::frameReady,
                    thumbnail_label, [thumbnail_label, temp_decoder](const QImage &frame) {
                        const QImage scaled_frame = frame.scaled(
                            thumbnail_label->width(), thumbnail_label->height(),
                            Qt::KeepAspectRatio, Qt::SmoothTransformation);

                        QImage overlay_image(thumbnail_label->width(), thumbnail_label->height(), QImage::Format_RGB32);
                        overlay_image.fill(Qt::black);

                        QPainter painter(&overlay_image);
                        const int x = (thumbnail_label->width() - scaled_frame.width()) / 2;
                        const int y = (thumbnail_label->height() - scaled_frame.height()) / 2;
                        painter.drawImage(x, y, scaled_frame);

                        painter.setPen(Qt::NoPen);
                        painter.setBrush(QColor(255, 255, 255, 180));
                        painter.drawEllipse(QRect(thumbnail_label->width() / 2 - 30,
                                                  thumbnail_label->height() / 2 - 30, 60, 60));

                        painter.setBrush(QColor(0, 0, 0, 200));
                        QPolygon triangle;
                        triangle << QPoint(thumbnail_label->width() / 2 - 15, thumbnail_label->height() / 2 - 20);
                        triangle << QPoint(thumbnail_label->width() / 2 + 25, thumbnail_label->height() / 2);
                        triangle << QPoint(thumbnail_label->width() / 2 - 15, thumbnail_label->height() / 2 + 20);
                        painter.drawPolygon(triangle);

                        QMetaObject::invokeMethod(thumbnail_label, "setPixmap",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(QPixmap, QPixmap::fromImage(overlay_image)));

                        temp_decoder->Stop();
                    },
                    Qt::QueuedConnection);

            // first frame extracting
            temp_decoder->ExtractFirstFrame();

            QThread::msleep(500);

            temp_decoder->Stop();
            temp_decoder->wait(300);
        } catch (...) {
        }
    });
}

bool AttachmentPlaceholder::eventFilter(QObject *watched, QEvent *event) {
    if (watched == thumbnail_label_ && event->type() == QEvent::MouseButtonRelease) {
        if (ClickHandler_) {
            ClickHandler_();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void AttachmentPlaceholder::NotifyLoaded() {
    emit attachmentLoaded();
}
