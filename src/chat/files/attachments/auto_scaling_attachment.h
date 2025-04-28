#ifndef AUTO_SCALING_ATTACHMENT_H
#define AUTO_SCALING_ATTACHMENT_H

#include <QWidget>
#include <QGraphicsOpacityEffect>
#include <QDesktopWidget>
#include <QScreen>

#include "../gif/player/inline_gif_player.h"
#include "../image/displayer/image_viewer.h"

class AutoScalingAttachment : public QWidget {
    Q_OBJECT

public:
    AutoScalingAttachment(QWidget* content, QWidget* parent = nullptr);

    void setMaxAllowedSize(const QSize& maxSize);

    QSize contentOriginalSize() const;

    bool isScaled() const {
        return m_isScaled;
    }

    QWidget* content() const {
        return m_content;
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
    void checkAndScaleContent();

private:
    void updateInfoLabelPosition();

    QWidget* m_content;
    QWidget* m_contentContainer;
    QLabel* m_infoLabel;
    bool m_isScaled;
    QSize m_scaledSize; // Przechowuje obliczony rozmiar po skalowaniu
    QSize m_maxAllowedSize;
};

#endif // AUTO_SCALING_ATTACHMENT_H