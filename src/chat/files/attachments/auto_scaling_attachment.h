#ifndef AUTO_SCALING_ATTACHMENT_H
#define AUTO_SCALING_ATTACHMENT_H

#include <QScreen>

#include "../gif/player/inline_gif_player.h"
#include "../image/displayer/image_viewer.h"

class AutoScalingAttachment final : public QWidget {
    Q_OBJECT

public:
    explicit AutoScalingAttachment(QWidget* content, QWidget* parent = nullptr);

    void SetMaxAllowedSize(const QSize& max_size);

    QSize ContentOriginalSize() const;

    bool IsScaled() const {
        return is_scaled_;
    }

    QWidget* Content() const {
        return content_;
    }

    QSize sizeHint() const override;

signals:
    void clicked();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

    void enterEvent(QEvent* event) override;

    void leaveEvent(QEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private slots:
    void CheckAndScaleContent();

private:
    void UpdateInfoLabelPosition() const;

    QWidget* content_;
    QWidget* content_container_;
    QLabel* info_label_;
    bool is_scaled_;
    QSize scaled_size_;
    QSize max_allowed_size_;
};

#endif // AUTO_SCALING_ATTACHMENT_H