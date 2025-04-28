#ifndef FINGERPRINT_LAYER_H
#define FINGERPRINT_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QSvgRenderer>
#include <QVector>

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
    void loadRandomFingerprint();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void updateFingerprintScan(int progressValue);

    QLabel* m_fingerprintImage;
    QProgressBar* m_fingerprintProgress;
    QTimer* m_fingerprintTimer;

    QSvgRenderer* m_svgRenderer;
    QImage m_baseFingerprint;
    QStringList m_fingerprintFiles;
    QString m_currentFingerprint;
};

#endif // FINGERPRINT_LAYER_H