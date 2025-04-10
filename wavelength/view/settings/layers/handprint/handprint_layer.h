#ifndef HANDPRINT_LAYER_H
#define HANDPRINT_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>

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
    void generateRandomHandprint();
    bool eventFilter(QObject *obj, QEvent *event) override;

    QLabel* m_handprintImage;
    QProgressBar* m_handprintProgress;
    QTimer* m_handprintTimer;
};

#endif // HANDPRINT_LAYER_H