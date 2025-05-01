#ifndef HANDPRINT_LAYER_H
#define HANDPRINT_LAYER_H

#include "../security_layer.h"
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QSvgRenderer>

class HandprintLayer final : public SecurityLayer {
    Q_OBJECT

public:
    explicit HandprintLayer(QWidget *parent = nullptr);
    ~HandprintLayer() override;

    void Initialize() override;
    void Reset() override;

    private slots:
        void UpdateProgress();
        void ProcessHandprint(bool completed);

private:
    void LoadRandomHandprint();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void UpdateHandprintScan(int progress_value) const;

    QLabel* handprint_image_;
    QProgressBar* handprint_progress_;
    QTimer* handprint_timer_;

    QSvgRenderer* svg_renderer_;
    QImage base_handprint_;
    QStringList handprint_files_;
    QString current_handprint_;
};

#endif // HANDPRINT_LAYER_H