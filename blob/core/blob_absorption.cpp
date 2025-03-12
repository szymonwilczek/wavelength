#include "blob_absorption.h"

#include <QApplication>
#include <QTimer>
#include <QDebug>

BlobAbsorption::BlobAbsorption(QWidget* parent)
    : QObject(parent),
      m_parentWidget(parent)
{
}

BlobAbsorption::~BlobAbsorption()
{
    cleanupAnimations();
}

void BlobAbsorption::setScale(float scale)
{
    if (m_scale != scale) {
        m_scale = scale;
        emit redrawNeeded();
    }
}

void BlobAbsorption::setOpacity(float opacity)
{
    if (m_opacity != opacity) {
        m_opacity = opacity;
        emit redrawNeeded();
    }
}

void BlobAbsorption::setPulse(float pulse)
{
    if (m_pulse != pulse) {
        m_pulse = pulse;
        emit redrawNeeded();
    }
}

void BlobAbsorption::startBeingAbsorbed()
{
    qDebug() << "BlobAbsorption: Rozpoczęcie procesu bycia absorbowanym";
    m_isBeingAbsorbed = true;
    m_opacity = 1.0f;
    m_scale = 1.0f;
    
    cleanupAnimations();
    
    m_scaleAnimation = new QPropertyAnimation(this, "scale");
    m_scaleAnimation->setDuration(8000);  // 8 sekund
    m_scaleAnimation->setStartValue(1.0);
    m_scaleAnimation->setEndValue(0.1);
    m_scaleAnimation->setEasingCurve(QEasingCurve::InQuad);
    
    m_opacityAnimation = new QPropertyAnimation(this, "opacity");
    m_opacityAnimation->setDuration(8000);  // 8 sekund
    m_opacityAnimation->setStartValue(1.0);
    m_opacityAnimation->setEndValue(0.0);
    m_opacityAnimation->setEasingCurve(QEasingCurve::InQuad);
    
    m_scaleAnimation->start();
    m_opacityAnimation->start();
    
    emit redrawNeeded();
}

void BlobAbsorption::finishBeingAbsorbed()
{
    qDebug() << "BlobAbsorption: Zakończenie procesu bycia absorbowanym";
    m_isClosingAfterAbsorption = true;
    
    m_opacity = 0.0f;
    m_scale = 0.1f;
    emit redrawNeeded();
    
    QTimer::singleShot(500, [this]() {
        qDebug() << "Aplikacja zostanie zamknięta po absorpcji";
        emit absorptionFinished();
        QApplication::quit();
    });
}

void BlobAbsorption::cancelAbsorption()
{
    qDebug() << "BlobAbsorption: Anulowanie procesu bycia absorbowanym";
    m_isBeingAbsorbed = false;
    
    cleanupAnimations();
    
    m_scaleAnimation = new QPropertyAnimation(this, "scale");
    m_scaleAnimation->setDuration(500);  // 0.5 sekundy
    m_scaleAnimation->setStartValue(m_scale);
    m_scaleAnimation->setEndValue(1.0);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    
    m_opacityAnimation = new QPropertyAnimation(this, "opacity");
    m_opacityAnimation->setDuration(500);  // 0.5 sekundy
    m_opacityAnimation->setStartValue(m_opacity);
    m_opacityAnimation->setEndValue(1.0);
    m_opacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
    
    m_scaleAnimation->start();
    m_opacityAnimation->start();
    
    connect(m_scaleAnimation, &QPropertyAnimation::finished, this, [this]() {
        emit animationCompleted();
    });
    
    emit redrawNeeded();
}

void BlobAbsorption::startAbsorbing(const QString& targetId)
{
    qDebug() << "BlobAbsorption: Rozpoczęcie absorbowania innej instancji:" << targetId;
    m_isAbsorbing = true;
    m_absorptionTargetId = targetId;
    m_scale = 1.0f;
    m_opacity = 1.0f;
    
    cleanupAnimations();
    
    m_scaleAnimation = new QPropertyAnimation(this, "scale");
    m_scaleAnimation->setDuration(8000);  // 8 sekund
    m_scaleAnimation->setStartValue(1.0);
    m_scaleAnimation->setEndValue(1.8);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnimation->start();
    
    m_pulseAnimation = new QPropertyAnimation(this, "pulse");
    m_pulseAnimation->setDuration(800);  // krótszy czas = szybsze pulsowanie
    m_pulseAnimation->setStartValue(0.0);
    m_pulseAnimation->setEndValue(1.0);
    m_pulseAnimation->setLoopCount(-1);  // pętla nieskończona
    m_pulseAnimation->start();
    
    emit redrawNeeded();
}

void BlobAbsorption::finishAbsorbing(const QString& targetId)
{
    qDebug() << "BlobAbsorption: Zakończenie absorbowania instancji:" << targetId;
    m_isAbsorbing = false;
    
    cleanupAnimations();
    
    m_scaleAnimation = new QPropertyAnimation(this, "scale");
    m_scaleAnimation->setDuration(500);  // 0.5 sekundy
    m_scaleAnimation->setStartValue(m_scale);
    m_scaleAnimation->setEndValue(1.0);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnimation->start();
    
    m_absorptionTargetId.clear();
    emit redrawNeeded();
}

void BlobAbsorption::cancelAbsorbing(const QString& targetId)
{
    qDebug() << "BlobAbsorption: Anulowanie absorbowania instancji:" << targetId;
    m_isAbsorbing = false;
    
    cleanupAnimations();
    
    m_scaleAnimation = new QPropertyAnimation(this, "scale");
    m_scaleAnimation->setDuration(500);  // 0.5 sekundy
    m_scaleAnimation->setStartValue(m_scale);
    m_scaleAnimation->setEndValue(1.0);
    m_scaleAnimation->setEasingCurve(QEasingCurve::OutQuad);
    m_scaleAnimation->start();
    
    connect(m_scaleAnimation, &QPropertyAnimation::finished, this, [this]() {
        m_pulse = 0.0f;
        emit animationCompleted();
    });
    
    m_absorptionTargetId.clear();
    emit redrawNeeded();
}

void BlobAbsorption::updateAbsorptionProgress(float progress)
{
    if (m_isBeingAbsorbed) {
        m_scale = 1.0f - progress * 0.9f;
        m_opacity = 1.0f - progress;
        qDebug() << "Blob pochłaniany: skala=" << m_scale << "przezroczystość=" << m_opacity;
    }
    else if (m_isAbsorbing) {
        m_scale = 1.0f + progress * 0.3f;
        qDebug() << "Blob pochłaniający: skala=" << m_scale;
    }
    
    emit redrawNeeded();
}

void BlobAbsorption::cleanupAnimations()
{
    if (m_scaleAnimation) {
        m_scaleAnimation->stop();
        delete m_scaleAnimation;
        m_scaleAnimation = nullptr;
    }
    
    if (m_opacityAnimation) {
        m_opacityAnimation->stop();
        delete m_opacityAnimation;
        m_opacityAnimation = nullptr;
    }
    
    if (m_pulseAnimation) {
        m_pulseAnimation->stop();
        delete m_pulseAnimation;
        m_pulseAnimation = nullptr;
    }
}