#ifndef RETINA_SCAN_LAYER_H
#define RETINA_SCAN_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

class RetinaScanLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit RetinaScanLayer(QWidget *parent = nullptr);
    ~RetinaScanLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void updateScan();
    void finishScan();

private:
    void generateEyeImage();
    void startScanAnimation();

    QLabel* m_eyeImage;
    QLabel* m_scannerOverlay;
    QProgressBar* m_scanProgress;
    QTimer* m_scanTimer;
    QTimer* m_completeTimer;
    int m_scanLine;
};

#endif // RETINA_SCAN_LAYER_H