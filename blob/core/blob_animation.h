#ifndef BLOBANIMATION_H
#define BLOBANIMATION_H

#include <deque>
#include <QWidget>
#include <QTimer>
#include <QTime>
#include <QPointF>
#include <QColor>
#include <QResizeEvent>
#include <QEvent>
#include <vector>
#include <memory>
#include <QThread>
#include <QMutex>
#include <QDebug>
#include <QPropertyAnimation>

#include "blob_config.h"
#include "../physics/blob_physics.h"
#include "../rendering/blob_render.h"
#include "../states/blob_state.h"
#include "../states/idle_state.h"
#include "../states/moving_state.h"
#include "../states/resizing_state.h"
#include "blob_event_handler.h"
#include "blob_transition_manager.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

class BlobAnimation : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit BlobAnimation(QWidget *parent = nullptr);

    void handleResizeTimeout();

    void checkWindowPosition();

    ~BlobAnimation() override;

    QPointF getBlobCenter() const {
        if (m_controlPoints.empty()) {
            return QPointF(width() / 2.0, height() / 2.0);
        }

        return m_blobCenter;
    }

    void applyExternalForce(const QVector2D& force) {
        qDebug() << "Stosowanie siły zewnętrznej do bloba:" << force;
        // TODO
    }

    void setLifeColor(const QColor& color);
    void resetLifeColor();

    void pauseAllEventTracking();
    void resumeAllEventTracking();

    void resetVisualization() {
        // Całkowicie resetujemy bloba do stanu początkowego
        // z oryginalnym rozmiarem i pozycją na środku
        resetBlobToCenter();

        // Wymuś reset i reinicjalizację HUD
        m_renderer.resetHUD();
        m_renderer.forceHUDInitialization(m_blobCenter, m_params.blobRadius,
                                        m_params.borderColor, width(), height());

        // Sygnał informujący o konieczności aktualizacji innych elementów UI
        emit visualizationReset();

        // Odśwież widok
        update();
    }

    public slots:
    void show() {
        if (!isVisible()) {
            setVisible(true);
            resumeAllEventTracking();
        }
    }

    void hide() {
        if (isVisible()) {
            setVisible(false);
            pauseAllEventTracking();
        }
    }

    void setBackgroundColor(const QColor &color);

    void setBlobColor(const QColor &color);

    void setGridColor(const QColor &color);

    void setGridSpacing(int spacing);

    signals:
    void visualizationReset();


protected:
    void paintEvent(QPaintEvent *event) override;

    QRectF calculateBlobBoundingRect();

    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void updateBlobGeometry();

    void drawGrid();

private slots:
    void updateAnimation();

    void processMovementBuffer();

    void updatePhysics();


    void onStateResetTimeout();

private:
    void initializeBlob();

    void switchToState(BlobConfig::AnimationState newState);

    void applyForces(const QVector2D &force);

    void applyIdleEffect();

    void resetBlobToCenter() {
        // Zapamiętaj aktualny promień bloba
        double originalRadius = m_params.blobRadius;

        // Całkowicie zresetuj bloba
        m_controlPoints.clear();
        m_targetPoints.clear();
        m_velocity.clear();

        // Ustaw bloba dokładnie na środku
        m_blobCenter = QPointF(width() / 2.0, height() / 2.0);

        // Generuj punkty kontrolne w nieregularnym, organicznym kształcie
        m_controlPoints = generateOrganicShape(m_blobCenter, originalRadius, m_params.numPoints);
        m_targetPoints = m_controlPoints;
        m_velocity.resize(m_params.numPoints, QPointF(0, 0));

        // Przełącz na stan IDLE i zresetuj zachowanie
        if (m_idleState) {
            static_cast<IdleState*>(m_idleState.get())->resetInitialization();
        }
        switchToState(BlobConfig::IDLE);
    }

    std::vector<QPointF> generateOrganicShape(const QPointF& center, double baseRadius, int numPoints) {
    std::vector<QPointF> points;
    points.reserve(numPoints);

    // Bardziej umiarkowane parametry deformacji - bliżej okręgu
    double minDeform = 0.9;  // Mniejsze wcięcia (90% oryginalnego promienia)
    double maxDeform = 1.1;  // Mniejsze wypukłości (110% oryginalnego promienia)

    // Generujemy wieloczęstotliwościowe zakłócenia - z mniejszą amplitudą
    std::vector<double> randomFactors(numPoints, 1.0);

    // Komponent losowych zakłóceń (wysokiej częstotliwości)
    for (int i = 0; i < numPoints; ++i) {
        // Mniejszy zakres losowości
        double baseFactor = minDeform + QRandomGenerator::global()->generateDouble() * (maxDeform - minDeform);
        randomFactors[i] = baseFactor;
    }

    // Dodajemy komponenty o niższych częstotliwościach - ale z mniejszą amplitudą
    int lobes = 2 + QRandomGenerator::global()->bounded(3); // 2-4 główne płaty
    double lobePhase = QRandomGenerator::global()->generateDouble() * M_PI * 2; // Losowe przesunięcie fazy

    for (int i = 0; i < numPoints; ++i) {
        double angle = 2.0 * M_PI * i / numPoints;
        // Mniejsze falowanie dla głównych płatów
        double lobeFactor = 0.07 * sin(angle * lobes + lobePhase);
        // Mniejsze falowanie dla dodatkowych nieregularności
        double midFactor = 0.04 * sin(angle * (lobes*2+1) + lobePhase * 1.5);

        // Łączymy wszystkie komponenty
        randomFactors[i] = randomFactors[i] + lobeFactor + midFactor;

        // Węższy dopuszczalny zakres
        randomFactors[i] = qBound(0.85, randomFactors[i], 1.15);
    }

    // Więcej przejść wygładzania dla bardziej płynnego kształtu
    std::vector<double> smoothedFactors = randomFactors;
    for (int smoothPass = 0; smoothPass < 3; ++smoothPass) {
        std::vector<double> tempFactors = smoothedFactors;
        for (int i = 0; i < numPoints; ++i) {
            int prev = (i > 0) ? i - 1 : numPoints - 1;
            int next = (i < numPoints - 1) ? i + 1 : 0;
            // Większa waga dla centralnego punktu = większa regularność
            smoothedFactors[i] = (tempFactors[prev] * 0.2 +
                                  tempFactors[i] * 0.6 +
                                  tempFactors[next] * 0.2);
        }
    }

    // Dodatkowe sprawdzenie maksymalnego promienia
    double maxAllowedRadius = baseRadius * 0.98; // Pozostawiamy 2% marginesu dla bezpieczeństwa

    // Generujemy punkty finalne
    for (int i = 0; i < numPoints; ++i) {
        double angle = 2.0 * M_PI * i / numPoints;
        // Skalowanie promienia aby nigdy nie przekroczyć bazowego promienia
        double deformedRadius = baseRadius * smoothedFactors[i];
        if (deformedRadius > maxAllowedRadius) {
            deformedRadius = maxAllowedRadius;
        }

        points.push_back(QPointF(
            center.x() + deformedRadius * cos(angle),
            center.y() + deformedRadius * sin(angle)
        ));
    }

    return points;
}

    BlobEventHandler m_eventHandler;
    BlobTransitionManager m_transitionManager;

    QTimer m_windowPositionTimer;
    QPointF m_lastWindowPosForTimer;
    QTimer m_resizeDebounceTimer;
    QSize m_lastSize;
    QColor m_defaultLifeColor;
    bool m_originalBorderColorSet = false;

    BlobConfig::BlobParameters m_params;
    BlobConfig::PhysicsParameters m_physicsParams;
    BlobConfig::IdleParameters m_idleParams;

    std::vector<QPointF> m_controlPoints;
    std::vector<QPointF> m_targetPoints;
    std::vector<QPointF> m_velocity;

    QPointF m_blobCenter;

    BlobConfig::AnimationState m_currentState = BlobConfig::IDLE;



    QTimer m_animationTimer;
    QTimer m_idleTimer;
    QTimer m_stateResetTimer;
    QTimer* m_transitionToIdleTimer = nullptr;

    BlobPhysics m_physics;
    BlobRenderer m_renderer;

    std::unique_ptr<IdleState> m_idleState;
    std::unique_ptr<MovingState> m_movingState;
    std::unique_ptr<ResizingState> m_resizingState;
    BlobState* m_currentBlobState;

    QPointF m_lastWindowPos;


    double m_precalcMinDistance = 0.0;
    double m_precalcMaxDistance = 0.0;

    QMutex m_dataMutex;
    std::atomic<bool> m_physicsRunning{true};
    std::vector<QPointF> m_safeControlPoints;

    bool m_eventsEnabled = true;
    QTimer m_eventReEnableTimer;
    bool m_needsRedraw = false;

    std::thread m_physicsThread;
    std::atomic<bool> m_physicsActive{true};
    std::mutex m_pointsMutex;
    std::condition_variable m_physicsCondition;
    bool m_needsUpdate{false};

    void physicsThreadFunction();

    QOpenGLShaderProgram* m_shaderProgram = nullptr;
    QOpenGLBuffer m_vbo;
    QOpenGLVertexArrayObject m_vao;

    // Bufor dla geometrii bloba
    std::vector<GLfloat> m_glVertices;
};

#endif // BLOBANIMATION_H