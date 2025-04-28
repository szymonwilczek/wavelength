#ifndef CYBER_ATTACHMENT_VIEWER_H
#define CYBER_ATTACHMENT_VIEWER_H

#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

#include "../chat/effects/mask_overlay.h"

class CyberAttachmentViewer final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int decryptionCounter READ decryptionCounter WRITE setDecryptionCounter)

public:
    explicit CyberAttachmentViewer(QWidget* parent = nullptr);

    ~CyberAttachmentViewer() override;

    int decryptionCounter() const { return m_decryptionCounter; }
    void setDecryptionCounter(int counter);

    void updateContentLayout();


    void setContent(QWidget* content);

    QSize sizeHint() const override;

protected:

    void resizeEvent(QResizeEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private slots:
    void onActionButtonClicked();

    void startScanningAnimation();

    void startDecryptionAnimation();

    void updateAnimation() const;

    void updateDecryptionStatus() const;

    void finishDecryption();

    void closeViewer();

signals:
    void decryptionCounterChanged(int value);
    void viewingFinished();

private:
    int m_decryptionCounter;
    bool m_isScanning;
    bool m_isDecrypted;

    QVBoxLayout* m_layout;
    QWidget* m_contentContainer;
    QVBoxLayout* m_contentLayout;
    QWidget* m_contentWidget = nullptr;
    QLabel* m_statusLabel;
    QTimer* m_animTimer;
    QTimer* m_blinkTimer = nullptr;
    int m_scanProgress = 0;
    MaskOverlay* m_maskOverlay;
};

#endif // CYBER_ATTACHMENT_VIEWER_H