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
    void setRevealProgress(int percentage);

    void startScanning();

    void stopScanning();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void updateScanLine();

private:
    int m_revealPercentage;
    int m_scanLineY;
    QTimer* m_scanTimer;
};

#endif // MASK_OVERLAY_H