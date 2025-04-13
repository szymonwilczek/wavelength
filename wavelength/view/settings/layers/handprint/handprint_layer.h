#ifndef HANDPRINT_LAYER_H
#define HANDPRINT_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QSvgRenderer>
#include <QVector>

class HandprintLayer : public SecurityLayer {
    Q_OBJECT

public:
    explicit HandprintLayer(QWidget *parent = nullptr);
    ~HandprintLayer() override;

    void initialize() override;
    void reset() override;

    private slots:
        void updateProgress();
        void processHandprint(bool completed);

private:
    void loadRandomHandprint();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void updateHandprintScan(int progressValue);

    QLabel* m_handprintImage;
    QProgressBar* m_handprintProgress;
    QTimer* m_handprintTimer;

    QSvgRenderer* m_svgRenderer;
    QImage m_baseHandprint;
    QStringList m_handprintFiles;
    QString m_currentHandprint;
};

#endif // HANDPRINT_LAYER_H