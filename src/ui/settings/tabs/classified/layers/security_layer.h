#ifndef SECURITY_LAYER_H
#define SECURITY_LAYER_H

#include <QWidget>

/**
 * @brief Abstract base class for individual security challenge layers.
 *
 * This class defines the common interface for different security verification steps
 * (like fingerprint scan, typing test, etc.). Each derived class implements a specific
 * challenge. It provides signals to indicate completion or failure of the challenge.
 */
class SecurityLayer : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Constructs a SecurityLayer.
     * @param parent Optional parent widget.
     */
    explicit SecurityLayer(QWidget *parent = nullptr) : QWidget(parent) {}

    /**
     * @brief Virtual destructor. Ensures proper cleanup for derived classes.
     */
    ~SecurityLayer() override = default;

    /**
     * @brief Pure virtual function to initialize the security layer for display and interaction.
     * Derived classes must implement this to set up their specific challenge state
     * (e.g., load images, generate codes, start timers).
     */
    virtual void Initialize() = 0;

    /**
     * @brief Pure virtual function to reset the security layer to its initial, inactive state.
     * Derived classes must implement this to clear inputs, reset progress, stop timers, etc.
     */
    virtual void Reset() = 0;

    signals:
        /**
         * @brief Emitted when the user successfully completes the security challenge presented by this layer.
         */
        void layerCompleted();

    /**
     * @brief Emitted when the user fails the security challenge (e.g., incorrect code, timeout, collision).
     * Note: Not all layers currently implement failure conditions that emit this signal.
     */
    void layerFailed();
};

#endif // SECURITY_LAYER_H