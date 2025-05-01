#ifndef ATTACHMENT_PLACEHOLDER_H
#define ATTACHMENT_PLACEHOLDER_H
#include <qfuture.h>
#include <QScrollArea>
#include <QWidget>
#include <QWindow>

#include "attachment_queue_manager.h"
#include "../gif/player/inline_gif_player.h"
#include "../video/player/video_player_overlay.h"

class AttachmentPlaceholder final : public QWidget {
    Q_OBJECT

public:
     AttachmentPlaceholder(const QString& filename, const QString& type, QWidget* parent = nullptr);

    void SetContent(QWidget* content);

    QSize sizeHint() const override;

    void SetAttachmentReference(const QString& attachment_id, const QString& mime_type);

    void SetBase64Data(const QString& base64_data, const QString& mime_type);

    void SetLoading(bool loading) const;


signals:
    void attachmentLoaded();

private slots:
    void onLoadButtonClicked();

public slots:

    void SetError(const QString& error_msg);

    void ShowFullSizeDialog(const QByteArray& data, bool is_gif);

    void AdjustAndShowDialog(QDialog* dialog, const QScrollArea* scroll_area, QWidget* content_widget, QSize original_content_size) const;

    void ShowCyberImage(const QByteArray& data);

    void ShowCyberGif(const QByteArray& data);

    void ShowCyberAudio(const QByteArray& data);

    void ShowCyberVideo(const QByteArray& data);

    void GenerateThumbnail(const QByteArray& video_data, QLabel* thumbnail_label);

private:
    QString filename_;
    QString base64_data_;
    QString mime_type_;
    QLabel* info_label_;
    QLabel* progress_label_;
    QPushButton* load_button_;
    QWidget* content_container_;
    QVBoxLayout* content_layout_;
    bool is_loaded_;
    QByteArray video_data_;
    QLabel* thumbnail_label_ = nullptr;
    std::function<void()> ClickHandler_;
    QString attachment_id_;
    bool has_reference_ = false;

    bool eventFilter(QObject* watched, QEvent* event) override;

    void NotifyLoaded();
};

#endif //ATTACHMENT_PLACEHOLDER_H
