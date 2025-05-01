#ifndef MASK_OVERLAY_H
#define MASK_OVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>

class MaskOverlay final : public QWidget {
    Q_OBJECT

public:
    explicit MaskOverlay(QWidget* parent = nullptr);

public slots:
    void SetRevealProgress(int percentage);

    void StartScanning();

    void StopScanning();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void UpdateScanLine();

private:
    int reveal_percentage_;
    int scanline_y_;
    QTimer* scan_timer_;
};

#endif // MASK_OVERLAY_H