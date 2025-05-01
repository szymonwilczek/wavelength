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

    int GetDecryptionCounter() const { return decryption_counter_; }
    void SetDecryptionCounter(int counter);

    void UpdateContentLayout();

    void SetContent(QWidget* content);

    QSize sizeHint() const override;

protected:

    void resizeEvent(QResizeEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private slots:
    void OnActionButtonClicked();

    void StartScanningAnimation();

    void StartDecryptionAnimation();

    void UpdateAnimation() const;

    void UpdateDecryptionStatus() const;

    void FinishDecryption();

    void CloseViewer();

signals:
    void decryptionCounterChanged(int value);
    void viewingFinished();

private:
    int decryption_counter_;
    bool is_scanning_;
    bool is_decrypted_;

    QVBoxLayout* layout_;
    QWidget* content_container_;
    QVBoxLayout* content_layout_;
    QWidget* content_widget_ = nullptr;
    QLabel* status_label_;
    QTimer* animation_timer_;
    QTimer* blink_timer_ = nullptr;
    int scan_progress_ = 0;
    MaskOverlay* mask_overlay_;
};

#endif // CYBER_ATTACHMENT_VIEWER_H