#include "blobanimation.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QtMath>
#include <QDebug>

BlobAnimation::BlobAnimation(QWidget *parent)
    : QWidget(parent),
      m_blobRadius(200.0f),
      m_numPoints(24),
      m_borderColor(QColor(70, 130, 180)),
      m_glowRadius(10),
      m_borderWidth(6)
{

    connect(&m_idleTimer, &QTimer::timeout, this, [this]() {
        if (m_currentState == IDLE) {
            applyIdleEffect();
        }
    });
    m_idleTimer.start(16);


    connect(&m_stateResetTimer, &QTimer::timeout, this, [this]() {
        switchToState(IDLE);
    });

    m_controlPoints.resize(m_numPoints);
    m_targetPoints.resize(m_numPoints);
    m_velocity.resize(m_numPoints);


    setAttribute(Qt::WA_TranslucentBackground);


    connect(&m_animationTimer, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    m_animationTimer.start(16);


    m_physicsTimer.start();


    initializeBlob();


    if (window())
        m_lastWindowPos = window()->pos();
}

BlobAnimation::~BlobAnimation() {
    m_animationTimer.stop();
}

void BlobAnimation::initializeBlob() {
    m_blobCenter = QPointF(width() / 2.0, height() / 2.0);


    for (int i = 0; i < m_numPoints; ++i) {
        double angle = 2 * M_PI * i / m_numPoints;



        double randomRadius = m_blobRadius * (0.9 + 0.2 * (qrand() % 100) / 100.0);

        QPointF point(
            m_blobCenter.x() + randomRadius * qCos(angle),
            m_blobCenter.y() + randomRadius * qSin(angle)
        );

        m_controlPoints[i] = point;
        m_targetPoints[i] = point;
        m_velocity[i] = QPointF(0, 0);
    }
}

void BlobAnimation::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);



    painter.fillRect(rect(), m_backgroundColor);


    painter.setPen(QPen(m_gridColor, 1, Qt::SolidLine));


    for (int y = 0; y < height(); y += m_gridSpacing) {
        painter.drawLine(0, y, width(), y);
    }


    for (int x = 0; x < width(); x += m_gridSpacing) {
        painter.drawLine(x, 0, x, height());
    }

    QPainterPath blobPath = createBlobPath();


    QColor glowColor = m_borderColor;
    glowColor.setAlpha(80);


    for (int i = m_glowRadius; i > 0; i -= 2) {
        QPen glowPen(glowColor);
        glowPen.setWidth(i);
        painter.setPen(glowPen);
        painter.drawPath(blobPath);
        glowColor.setAlpha(glowColor.alpha() - 15);
    }


    QPen borderPen(m_borderColor);
    borderPen.setWidth(m_borderWidth);
    painter.setPen(borderPen);
    painter.drawPath(blobPath);


    QRadialGradient gradient(m_blobCenter, m_blobRadius);
    QColor centerColor = m_borderColor;
    centerColor.setAlpha(30);
    gradient.setColorAt(0, centerColor);
    gradient.setColorAt(1, QColor(0, 0, 0, 0));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(blobPath);
}

QPainterPath BlobAnimation::createBlobPath() const {
    QPainterPath path;

    if (m_controlPoints.isEmpty()) return path;


    path.moveTo(m_controlPoints[0]);


    for (int i = 0; i < m_controlPoints.size(); ++i) {
        int prev = (i + m_numPoints - 1) % m_numPoints;
        int curr = i;
        int next = (i + 1) % m_numPoints;
        int nextNext = (i + 2) % m_numPoints;


        QPointF p0 = m_controlPoints[prev];
        QPointF p1 = m_controlPoints[curr];
        QPointF p2 = m_controlPoints[next];
        QPointF p3 = m_controlPoints[nextNext];

        if (!isValidPoint(p0) || !isValidPoint(p1) || !isValidPoint(p2) || !isValidPoint(p3)) {
            continue;
        }



        float tension = 0.2f;

        QPointF c1 = p1 + tension * (p2 - p0);
        QPointF c2 = p2 - tension * (p3 - p1);

        path.cubicTo(c1, c2, p2);
    }

    return path;
}

bool BlobAnimation::isValidPoint(const QPointF &point) const {

    if (std::isnan(point.x()) || std::isnan(point.y()) ||
        std::isinf(point.x()) || std::isinf(point.y())) {
        return false;
        }


    const double maxCoord = 100000.0;
    if (qAbs(point.x()) > maxCoord || qAbs(point.y()) > maxCoord) {
        return false;
    }

    return true;
}

void BlobAnimation::updateAnimation() {

    if (m_currentState == IDLE) {
        applyIdleEffect();
    }

    updateBlobPhysics();


    const double viscosity = 0.015;
    const double damping = 0.91;


    bool isInMotion = false;
    const double velocityThreshold = 0.1;

    for (int i = 0; i < m_numPoints; ++i) {

        QPointF force = (m_targetPoints[i] - m_controlPoints[i]) * viscosity;


        QPointF vectorToCenter = m_blobCenter - m_controlPoints[i];
        double distanceFromCenter = QVector2D(vectorToCenter).length();


        if (distanceFromCenter > m_blobRadius * 1.1) {

            force += vectorToCenter * 0.03;
        }


        m_velocity[i] += force;
        m_velocity[i] *= damping;


        double speed = QVector2D(m_velocity[i]).length();
        if (speed < velocityThreshold) {
            m_velocity[i] = QPointF(0, 0);
        } else {
            isInMotion = true;
        }


        const double maxSpeed = m_blobRadius * 0.05;
        if (speed > maxSpeed) {
            m_velocity[i] *= (maxSpeed / speed);
        }


        m_controlPoints[i] += m_velocity[i];
    }


    if (!isInMotion) {
        stabilizeBlob();
    }

    handleBorderCollisions();


    smoothBlobShape();


    constrainNeighborDistances();


    update();
}

void BlobAnimation::stabilizeBlob() {
    const double stabilizationRate = 0.001;

    for (int i = 0; i < m_numPoints; ++i) {

        double angle = 2 * M_PI * i / m_numPoints;
        QPointF idealPoint(
            m_blobCenter.x() + m_blobRadius * qCos(angle),
            m_blobCenter.y() + m_blobRadius * qSin(angle)
        );


        m_controlPoints[i] += (idealPoint - m_controlPoints[i]) * stabilizationRate;
    }
}


void BlobAnimation::constrainNeighborDistances() {
    const double minDistance = m_blobRadius * 0.1;
    const double maxDistance = m_blobRadius * 0.6;

    for (int i = 0; i < m_numPoints; ++i) {
        int next = (i + 1) % m_numPoints;

        QPointF diff = m_controlPoints[next] - m_controlPoints[i];
        double distance = QVector2D(diff).length();


        if (distance < minDistance || distance > maxDistance) {
            QPointF direction = diff / distance;
            double targetDistance = qBound(minDistance, distance, maxDistance);


            QPointF correction = direction * (distance - targetDistance) * 0.5;
            m_controlPoints[i] += correction;
            m_controlPoints[next] -= correction;


            if (distance < minDistance) {

                m_velocity[i] -= direction * 0.3;
                m_velocity[next] += direction * 0.3;
            } else if (distance > maxDistance) {

                m_velocity[i] += direction * 0.3;
                m_velocity[next] -= direction * 0.3;
            }
        }
    }
}

void BlobAnimation::handleBorderCollisions() {
    const double restitution = 0.2;

    int padding = m_borderWidth + m_glowRadius;


    int minX = padding;
    int minY = padding;
    int maxX = width() - padding;
    int maxY = height() - padding;


    for (int i = 0; i < m_numPoints; ++i) {

        if (m_controlPoints[i].x() < minX) {
            m_controlPoints[i].setX(minX);
            m_velocity[i].setX(-m_velocity[i].x() * restitution);
        }

        else if (m_controlPoints[i].x() > maxX) {
            m_controlPoints[i].setX(maxX);
            m_velocity[i].setX(-m_velocity[i].x() * restitution);
        }


        if (m_controlPoints[i].y() < minY) {
            m_controlPoints[i].setY(minY);
            m_velocity[i].setY(-m_velocity[i].y() * restitution);
        }

        else if (m_controlPoints[i].y() > maxY) {
            m_controlPoints[i].setY(maxY);
            m_velocity[i].setY(-m_velocity[i].y() * restitution);
        }
    }


    if (m_blobCenter.x() < minX) m_blobCenter.setX(minX);
    if (m_blobCenter.x() > maxX) m_blobCenter.setX(maxX);
    if (m_blobCenter.y() < minY) m_blobCenter.setY(minY);
    if (m_blobCenter.y() > maxY) m_blobCenter.setY(maxY);
}

void BlobAnimation::smoothBlobShape() {


    QVector<QPointF> smoothedPoints = m_controlPoints;

    for (int i = 0; i < m_numPoints; ++i) {
        int prev = (i + m_numPoints - 1) % m_numPoints;
        int next = (i + 1) % m_numPoints;


        QPointF neighborAverage = (m_controlPoints[prev] + m_controlPoints[next]) * 0.5;


        smoothedPoints[i] += (neighborAverage - m_controlPoints[i]) * 0.15;
    }

    m_controlPoints = smoothedPoints;
}

void BlobAnimation::updateBlobPhysics() {

    QWidget* windowWidget = window();
    if (!windowWidget)
        return;



    double deltaTime = m_physicsTimer.restart() / 1000.0;
    if (deltaTime < 0.001) {
        deltaTime = 0.016;
    } else if (deltaTime > 0.1) {
        deltaTime = 0.1;
    }


    QPointF currentWindowPos = windowWidget->pos();
    QVector2D windowVelocity(
        (currentWindowPos.x() - m_lastWindowPos.x()) / deltaTime,
        (currentWindowPos.y() - m_lastWindowPos.y()) / deltaTime
    );

    const double maxVelocity = 2000.0;
    if (windowVelocity.length() > maxVelocity) {
        windowVelocity = windowVelocity.normalized() * maxVelocity;
    }


    if (windowVelocity.length() > 1.0) {
        switchToState(MOVING);
        QVector2D inertiaForce = -windowVelocity * 0.0015;
        applyForces(inertiaForce);
    }


    m_lastWindowPos = currentWindowPos;
    m_lastWindowVelocity = windowVelocity;

    validateAndRepairControlPoints();
}

void BlobAnimation::validateAndRepairControlPoints() {
    bool hasInvalidPoints = false;


    for (int i = 0; i < m_numPoints; ++i) {
        if (!isValidPoint(m_controlPoints[i]) || !isValidPoint(m_velocity[i])) {
            hasInvalidPoints = true;
            break;
        }
    }


    if (hasInvalidPoints) {
        qDebug() << "Wykryto nieprawidłowe punkty kontrolne. Resetowanie kształtu plamy.";
        initializeBlob();
    }
}

void BlobAnimation::applyForces(const QVector2D &force) {

    for (int i = 0; i < m_numPoints; ++i) {

        QPointF vectorFromCenter = m_controlPoints[i] - m_blobCenter;
        double distanceFromCenter = QVector2D(vectorFromCenter).length();


        double forceScale = distanceFromCenter / m_blobRadius;


        if (forceScale > 1.0) forceScale = 1.0;


        m_velocity[i] += QPointF(
            force.x() * forceScale * 0.8,
            force.y() * forceScale * 0.8
        );
    }


    m_blobCenter += QPointF(force.x() * 0.2, force.y() * 0.2);
}

void BlobAnimation::resizeEvent(QResizeEvent *event) {
    switchToState(RESIZING);
    QPointF oldCenter = m_blobCenter;
    m_blobCenter = QPointF(width() / 2.0, height() / 2.0);


    QPointF delta = m_blobCenter - oldCenter;
    for (int i = 0; i < m_numPoints; ++i) {
        m_controlPoints[i] += delta;
        m_targetPoints[i] += delta;
    }


    if (event->oldSize().isValid()) {
        QVector2D resizeForce(
            (event->size().width() - event->oldSize().width()) * 0.05,
            (event->size().height() - event->oldSize().height()) * 0.05
        );
        applyForces(resizeForce);
    }

    QWidget::resizeEvent(event);
}

bool BlobAnimation::event(QEvent *event) {
    if (event->type() == QEvent::Move) {
        switchToState(MOVING);
    }
    else if (event->type() == QEvent::Resize) {
        switchToState(RESIZING);
    }

    return QWidget::event(event);
}

void BlobAnimation::setBackgroundColor(const QColor &color) {
    m_backgroundColor = color;
    update();
}

void BlobAnimation::setGridColor(const QColor &color) {
    m_gridColor = color;
    update();
}

void BlobAnimation::setGridSpacing(int spacing) {
    m_gridSpacing = spacing;
    update();
}

void BlobAnimation::applyIdleEffect() {
   
    m_idleWavePhase += 0.005; 
    if (m_idleWavePhase > 2 * M_PI) {
        m_idleWavePhase -= 2 * M_PI;
    }

    static double secondPhase = 0.0;
    secondPhase += 0.01; 
    if (secondPhase > 2 * M_PI) {
        secondPhase -= 2 * M_PI;
    }

   
    static double rotationPhase = 0.0;
    rotationPhase += 0.0025; 
    if (rotationPhase > 2 * M_PI) {
        rotationPhase -= 2 * M_PI;
    }

   
    double rotationStrength = 0.1 * std::sin(rotationPhase * 0.5); 

    for (int i = 0; i < m_numPoints; ++i) {
        QPointF vectorFromCenter = m_controlPoints[i] - m_blobCenter;
        double angle = std::atan2(vectorFromCenter.y(), vectorFromCenter.x());
        double distanceFromCenter = QVector2D(vectorFromCenter).length();

        double waveStrength = m_idleWaveAmplitude * 0.7 * 
            std::sin(m_idleWavePhase + m_idleWaveFrequency * angle);

       
        waveStrength += (m_idleWaveAmplitude * 0.4) * 
            std::sin(secondPhase + m_idleWaveFrequency * 2 * angle + 0.5);

       
        waveStrength += (m_idleWaveAmplitude * 0.3) * 
            std::sin(m_idleWavePhase * 0.7) * std::cos(angle * 3);

        QVector2D normalizedVector = QVector2D(vectorFromCenter).normalized();

       
        QPointF perpVector(-normalizedVector.y(), normalizedVector.x());

       
        double rotationFactor = rotationStrength * (distanceFromCenter / m_blobRadius);
        QPointF rotationForce = rotationFactor * perpVector;

       
        QPointF forceVector = QPointF(normalizedVector.x(), normalizedVector.y()) * waveStrength + rotationForce;

        double forceScale = 0.3 + 0.7 * (distanceFromCenter / m_blobRadius);
        if (forceScale > 1.0) forceScale = 1.0;

       
        m_velocity[i] += forceVector * forceScale * 0.15; 
    }
}

void BlobAnimation::switchToState(AnimationState newState) {
    if (m_currentState != newState) {
        m_currentState = newState;

        if (newState != IDLE) {
            m_stateResetTimer.stop();
        }

        if (newState == MOVING || newState == RESIZING) {
            m_stateResetTimer.start(1000);
        }
    }
}