#ifndef SECURITY_LAYER_H
#define SECURITY_LAYER_H

#include <QWidget>

class SecurityLayer : public QWidget {
    Q_OBJECT
public:
    explicit SecurityLayer(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~SecurityLayer() = default;

    // Metoda inicjalizująca warstwę zabezpieczeń
    virtual void initialize() = 0;

    // Resetuje stan warstwy zabezpieczeń
    virtual void reset() = 0;

    signals:
        void layerCompleted();
        void layerFailed();
};

#endif // SECURITY_LAYER_H