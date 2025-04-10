#ifndef FINGERPRINT_LAYER_H
#define FINGERPRINT_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

class FingerprintLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit FingerprintLayer(QWidget *parent = nullptr);
    ~FingerprintLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void updateProgress();
    void processFingerprint(bool completed);

private:
    void generateRandomFingerprint();
    bool eventFilter(QObject *obj, QEvent *event) override;

    QLabel* m_fingerprintImage;
    QProgressBar* m_fingerprintProgress;
    QTimer* m_fingerprintTimer;
};

#endif // FINGERPRINT_LAYER_H