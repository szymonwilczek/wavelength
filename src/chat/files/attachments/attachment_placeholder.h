#ifndef ATTACHMENT_PLACEHOLDER_H
#define ATTACHMENT_PLACEHOLDER_H
#include <qfileinfo.h>
#include <qfuture.h>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <qtconcurrentrun.h>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>

#include "attachment_queue_manager.h"
#include "auto_scaling_attachment.h"
#include "../../messages/formatter/message_formatter.h"
#include "../../../ui/files/cyber_attachment_viewer.h"
#include "../gif/player/inline_gif_player.h"
#include "../image/displayer/image_viewer.h"
#include "../video/player/video_player_overlay.h"

class AttachmentPlaceholder : public QWidget {
    Q_OBJECT

public:
     AttachmentPlaceholder(const QString& filename, const QString& type, QWidget* parent = nullptr, bool autoLoad = true);

    void setContent(QWidget* content);

    QSize sizeHint() const override;

    void setAttachmentReference(const QString& attachmentId, const QString& mimeType);

    void setBase64Data(const QString& base64Data, const QString& mimeType);

    void setLoading(bool loading);


signals:
    void attachmentLoaded();

private slots:
    void onLoadButtonClicked();

public slots:

    void setError(const QString& errorMsg);

    void showFullSizeDialog(const QByteArray& data, bool isGif);

    // Funkcja pomocnicza do dostosowania rozmiaru i pokazania dialogu
    void adjustAndShowDialog(QDialog* dialog, QScrollArea* scrollArea, QWidget* contentWidget, QSize originalContentSize);

    void showCyberImage(const QByteArray& data);

    void showCyberGif(const QByteArray& data);

    void showCyberAudio(const QByteArray& data);

    void showCyberVideo(const QByteArray& data);

    void showVideo(const QByteArray& data);

    void generateThumbnail(const QByteArray& videoData, QLabel* thumbnailLabel);

private:
    QString m_filename;
    QString m_base64Data;
    QString m_mimeType;
    QLabel* m_infoLabel;
    QLabel* m_progressLabel;
    QPushButton* m_loadButton;
    QWidget* m_contentContainer;
    QVBoxLayout* m_contentLayout;
    bool m_isLoaded;
    QByteArray m_videoData; // Do przechowywania danych wideo
    QLabel* m_thumbnailLabel = nullptr;
    std::function<void()> m_clickHandler;
    QString m_attachmentId;
    bool m_hasReference = false;

    bool eventFilter(QObject* watched, QEvent* event) override;

    void notifyLoaded();
};

#endif //ATTACHMENT_PLACEHOLDER_H
