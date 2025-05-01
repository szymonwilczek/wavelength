#ifndef RETINA_SCAN_LAYER_H
#define RETINA_SCAN_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QImage>

class RetinaScanLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit RetinaScanLayer(QWidget *parent = nullptr);
    ~RetinaScanLayer() override;

    void Initialize() override;
    void Reset() override;

    private slots:
        void UpdateScan();
        void FinishScan();

private:
    void GenerateEyeImage();
    void StartScanAnimation() const;

    QLabel* eye_image_;
    QProgressBar* scan_progress_;
    QTimer* scan_timer_;
    QTimer* complete_timer_;
    int scanline_;

    QImage base_eye_image_;
};

#endif // RETINA_SCAN_LAYER_H