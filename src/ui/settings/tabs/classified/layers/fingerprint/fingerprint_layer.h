#ifndef FINGERPRINT_LAYER_H
#define FINGERPRINT_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QSvgRenderer>

class FingerprintLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit FingerprintLayer(QWidget *parent = nullptr);
    ~FingerprintLayer() override;

    void Initialize() override;
    void Reset() override;

    private slots:
        void UpdateProgress();
        void ProcessFingerprint(bool completed);

private:
    void LoadRandomFingerprint();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void UpdateFingerprintScan(int progress_value) const;

    QLabel* fingerprint_image_;
    QProgressBar* fingerprint_progress_;
    QTimer* fingerprint_timer_;

    QSvgRenderer* svg_renderer_;
    QImage base_fingerprint_;
    QStringList fingerprint_files_;
    QString current_fingerprint_;
};

#endif // FINGERPRINT_LAYER_H