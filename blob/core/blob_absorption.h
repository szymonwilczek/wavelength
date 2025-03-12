#ifndef BLOB_ABSORPTION_H
#define BLOB_ABSORPTION_H

#include <QObject>
#include <QColor>
#include <QPropertyAnimation>
#include <QString>
#include <QWidget>

class BlobAbsorption : public QObject {
    Q_OBJECT
    Q_PROPERTY(float scale READ scale WRITE setScale)
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(float pulse READ pulse WRITE setPulse)

public:
    explicit BlobAbsorption(QWidget* parent = nullptr);
    ~BlobAbsorption();

    // Gettery i settery dla właściwości animacji
    float scale() const { return m_scale; }
    void setScale(float scale);

    float opacity() const { return m_opacity; }
    void setOpacity(float opacity);

    float pulse() const { return m_pulse; }
    void setPulse(float pulse);

    // Metody zarządzające stanem absorpcji
    void startBeingAbsorbed();
    void finishBeingAbsorbed();
    void cancelAbsorption();

    void startAbsorbing(const QString& targetId);
    void finishAbsorbing(const QString& targetId);
    void cancelAbsorbing(const QString& targetId);

    void updateAbsorptionProgress(float progress);

    // Gettery stanu
    bool isBeingAbsorbed() const { return m_isBeingAbsorbed; }
    bool isAbsorbing() const { return m_isAbsorbing; }
    bool isClosingAfterAbsorption() const { return m_isClosingAfterAbsorption; }
    bool isPulseActive() const { return m_pulse > 0.0f; }

    signals:
        void redrawNeeded();
    void animationCompleted();
    void absorptionFinished();

private:
    QWidget* m_parentWidget;

    float m_scale = 1.0f;
    float m_opacity = 1.0f;
    float m_pulse = 0.0f;

    QPropertyAnimation* m_scaleAnimation = nullptr;
    QPropertyAnimation* m_opacityAnimation = nullptr;
    QPropertyAnimation* m_pulseAnimation = nullptr;

    bool m_isBeingAbsorbed = false;
    bool m_isAbsorbing = false;
    bool m_isClosingAfterAbsorption = false;
    QString m_absorptionTargetId;

    void cleanupAnimations();
};

#endif // BLOB_ABSORPTION_H