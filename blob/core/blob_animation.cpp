#include "blob_animation.h"

#include <QApplication>
#include <QDebug>

#include "../../wavelength/util/wavelength_config.h"

BlobAnimation::BlobAnimation(QWidget *parent)
    : QOpenGLWidget(parent),
      m_transitionManager(this),
      m_eventHandler(this) {

    WavelengthConfig *config = WavelengthConfig::getInstance();

    // --- Zainicjuj parametry bloba z konfiguracji ---
    m_params.backgroundColor = config->getBackgroundColor();
    m_params.borderColor = config->getBlobColor(); // Zakładamy, że borderColor to główny kolor bloba
    m_params.gridColor = config->getGridColor();
    m_params.gridSpacing = config->getGridSpacing();

    m_params.blobRadius = 250.0f;
    m_params.numPoints = 32;
    // m_params.borderColor = QColor(0, 200, 255); // Neonowy niebieski
    // m_params.backgroundColor = QColor(0, 15, 30); // Ciemny odcień granatowego
    // m_params.gridColor = QColor(0, 100, 150, 60); // Neonowa siatka
    m_params.glowRadius = 10; // Większy promień poświaty
    m_params.borderWidth = 3; // Grubość obramowania
    // m_params.gridSpacing = 20; // Odstęp siatki

    m_idleParams.waveAmplitude = 2.0; // Nieco większa amplituda
    m_idleParams.waveFrequency = 2.0;

    m_idleState = std::make_unique<IdleState>();
    m_movingState = std::make_unique<MovingState>();
    m_resizingState = std::make_unique<ResizingState>();
    m_currentBlobState = m_idleState.get();

    m_controlPoints.reserve(m_params.numPoints);
    m_targetPoints.reserve(m_params.numPoints);
    m_velocity.reserve(m_params.numPoints);


    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, true);

    initializeBlob();

    m_physicsThread = std::thread(&BlobAnimation::physicsThreadFunction, this);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    m_resizeDebounceTimer.setSingleShot(true);
    m_resizeDebounceTimer.setInterval(50); // 50 ms debounce
    connect(&m_resizeDebounceTimer, &QTimer::timeout, this, &BlobAnimation::handleResizeTimeout);

    if (window()) {
        m_lastWindowPos = window()->pos();
        m_lastWindowPosForTimer = m_lastWindowPos;
    }

    m_animationTimer.setTimerType(Qt::PreciseTimer);
    m_animationTimer.setInterval(16); // ~60 FPS
    connect(&m_animationTimer, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    m_animationTimer.start();

    m_windowPositionTimer.setInterval(16); // (>100 FPS)
    m_windowPositionTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_windowPositionTimer, &QTimer::timeout, this, &BlobAnimation::checkWindowPosition);
    m_windowPositionTimer.start();

    connect(&m_stateResetTimer, &QTimer::timeout, this, &BlobAnimation::onStateResetTimeout);

    // Podłączenie eventów z BlobEventHandler
    connect(&m_eventHandler, &BlobEventHandler::windowMoved, this, [this](const QPointF &pos, qint64 timestamp) {
        m_lastWindowPosForTimer = pos;
    });

    connect(&m_eventHandler, &BlobEventHandler::movementSampleAdded, this,
            [this](const QPointF &pos, qint64 timestamp) {
                m_transitionManager.addMovementSample(pos, timestamp);
            });

    connect(&m_eventHandler, &BlobEventHandler::resizeStateRequested, this, [this]() {
        switchToState(BlobConfig::RESIZING);
    });

    connect(&m_eventHandler, &BlobEventHandler::significantResizeDetected, this,
            [this](const QSize &oldSize, const QSize &newSize) {
                m_resizingState->handleResize(m_controlPoints, m_targetPoints, m_velocity,
                                              m_blobCenter, oldSize, newSize);

                // Resetuj bufor siatki tylko gdy zmiana rozmiaru jest znacząca
                if (abs(newSize.width() - m_lastSize.width()) > 20 ||
                    abs(newSize.height() - m_lastSize.height()) > 20) {
                    m_renderer.resetGridBuffer();
                }

                m_lastSize = newSize;
                // update();
            });

    connect(&m_eventHandler, &BlobEventHandler::stateResetTimerRequested, this, [this]() {
        m_stateResetTimer.stop();
        m_stateResetTimer.start(2000);
    });

    connect(&m_eventHandler, &BlobEventHandler::eventsReEnabled, this, [this]() {
        m_eventsEnabled = true;
        qDebug() << "Events re-enabled via handler";
    });

    connect(&m_transitionManager, &BlobTransitionManager::transitionCompleted, this, [this]() {
        m_eventReEnableTimer.start(200);
    });

    connect(&m_transitionManager, &BlobTransitionManager::significantMovementDetected, this, [this]() {
        switchToState(BlobConfig::MOVING);
        m_needsRedraw = true;
    });

    connect(&m_transitionManager, &BlobTransitionManager::movementStopped, this, [this]() {
        if (m_currentState == BlobConfig::MOVING) {
            switchToState(BlobConfig::IDLE);
        }
    });

    m_eventReEnableTimer.setSingleShot(true);
    connect(&m_eventReEnableTimer, &QTimer::timeout, this, [this]() {
        m_eventsEnabled = true;
        m_eventHandler.enableEvents();
        qDebug() << "Events re-enabled";
    });
}

void BlobAnimation::handleResizeTimeout() {
    // Po każdym zdarzeniu resize, resetujemy bloba do oryginalnego rozmiaru i środka
    resetBlobToCenter();

    // Resetujemy bufor siatki
    m_renderer.resetGridBuffer();
    m_lastSize = size();

    // Odświeżamy widok
    update();
}

void BlobAnimation::checkWindowPosition() {
    QWidget *currentWindow = window();
    if (!m_eventsEnabled || !currentWindow)
        return;

    static qint64 lastCheckTime = 0;
    qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();

    if (currentTimestamp - lastCheckTime < 16) {
        return;
    }

    lastCheckTime = currentTimestamp;

    std::pmr::deque<BlobTransitionManager::WindowMovementSample> movementBuffer = m_transitionManager.
            getMovementBuffer();

    if (movementBuffer.empty() ||
        (currentTimestamp - m_transitionManager.getLastMovementTime()) > 100) {
        QPointF currentWindowPos = currentWindow->pos();

        if (movementBuffer.empty() || currentWindowPos != movementBuffer.back().position) {
            m_transitionManager.addMovementSample(currentWindowPos, currentTimestamp);
        }
    }
}

void BlobAnimation::physicsThreadFunction() {
    while (m_physicsActive) {
        auto startTime = std::chrono::high_resolution_clock::now(); {
            std::lock_guard<std::mutex> lock(m_pointsMutex);
            // Wywołaj swoją istniejącą metodę fizyki - dostosuj nazwę jeśli potrzeba
            updatePhysics();
        }

        // Poinformuj wątek UI o konieczności odświeżenia
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

        // Utrzymuj stałą liczbę klatek
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        auto targetFrameTime = std::chrono::microseconds(8333); // ~120fps

        if (duration < targetFrameTime) {
            std::unique_lock<std::mutex> lock(m_pointsMutex);
            m_physicsCondition.wait_for(lock, targetFrameTime - duration);
        }
    }
}

BlobAnimation::~BlobAnimation() {
    m_physicsActive = false;
    m_physicsCondition.notify_all();
    if (m_physicsThread.joinable()) {
        m_physicsThread.join();
    }

    makeCurrent();
    m_vbo.destroy();
    m_vao.destroy();
    delete m_shaderProgram;
    doneCurrent();

    m_animationTimer.stop();
    m_idleTimer.stop();
    m_stateResetTimer.stop();

    m_idleState.reset();
    m_movingState.reset();
    m_resizingState.reset();
}

void BlobAnimation::initializeBlob() {
    std::lock_guard<std::mutex> lock(m_pointsMutex);
    // Wyczyść aktualne punkty
    m_controlPoints.clear();
    m_targetPoints.clear();
    m_velocity.clear();

    // Ustaw bloba na środek ekranu
    m_blobCenter = QPointF(width() / 2.0, height() / 2.0);

    // Generuj punkty kontrolne w nieregularnym, organicznym kształcie
    m_controlPoints = generateOrganicShape(m_blobCenter, m_params.blobRadius, m_params.numPoints);

    // Ustawiamy punkty docelowe i prędkości
    m_targetPoints = m_controlPoints;
    m_velocity.resize(m_params.numPoints, QPointF(0, 0));

    // To zapewni, że nawet jeśli parametry fizyki będą próbować
    // zmienić rozmiar, to my go zawsze resetujemy do określonej wartości
    m_precalcMinDistance = m_params.blobRadius * m_physicsParams.minNeighborDistance;
    m_precalcMaxDistance = m_params.blobRadius * m_physicsParams.maxNeighborDistance;
}

void BlobAnimation::paintEvent(QPaintEvent *event) {
    std::lock_guard lock(m_pointsMutex);

    QRectF blobRect = calculateBlobBoundingRect();
    if (!event->rect().intersects(blobRect.toRect())) {
        return;
    }

    static QPixmap backgroundCache;
    static QSize lastBackgroundSize;
    static QColor lastBgColor;
    static QColor lastGridColor;
    static int lastGridSpacing;

    QPainter painter(this);

    // Przygotuj stan renderowania
    BlobRenderState renderState;
    renderState.animationState = m_currentState;

    // Deleguj renderowanie do BlobRenderer
    m_renderer.renderScene(
        painter,
        m_controlPoints,
        m_blobCenter,
        m_params,
        renderState,
        width(),
        height(),
        backgroundCache,
        lastBackgroundSize,
        lastBgColor,
        lastGridColor,
        lastGridSpacing
    );
}

QRectF BlobAnimation::calculateBlobBoundingRect() {
    if (m_controlPoints.empty()) {
        return QRectF(0, 0, width(), height());
    }

    auto xComp = [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); };
    auto yComp = [](const QPointF &a, const QPointF &b) { return a.y() < b.y(); };

    auto [minXIt, maxXIt] = std::minmax_element(m_controlPoints.begin(), m_controlPoints.end(), xComp);
    auto [minYIt, maxYIt] = std::minmax_element(m_controlPoints.begin(), m_controlPoints.end(), yComp);

    qreal minX = minXIt->x();
    qreal maxX = maxXIt->x();
    qreal minY = minYIt->y();
    qreal maxY = maxYIt->y();

    int margin = m_params.borderWidth + m_params.glowRadius + 5;
    return QRectF(minX - margin, minY - margin,
                  maxX - minX + 2 * margin, maxY - minY + 2 * margin);
}


void BlobAnimation::updateAnimation() {
    m_needsRedraw = false;

    { // <<< Dodaj zakres dla blokady
        std::lock_guard<std::mutex> lock(m_pointsMutex); // <<< DODAJ BLOKADĘ

        // processMovementBuffer może modyfikować wektory przez applyInertiaForce
        processMovementBuffer();

        // Stany IDLE/MOVING/RESIZING mogą modyfikować wektory w metodzie apply
        if (m_currentState == BlobConfig::IDLE) {
            if (m_currentBlobState) {
                m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
            }
        } else if (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING) {
            if (m_currentBlobState) {
                m_currentBlobState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
                m_needsRedraw = true;
            }
        }
    } // <<< Blokada zwalniana automatycznie

    if (m_needsRedraw) {
        update();
        m_params.screenWidth = width();
        m_params.screenHeight = height();
    }
}

void BlobAnimation::processMovementBuffer() {
    m_transitionManager.processMovementBuffer(
        m_velocity,
        m_blobCenter,
        m_controlPoints,
        m_params.blobRadius,
        [this](std::vector<QPointF> &vel, QPointF &center, std::vector<QPointF> &points, float radius,
               QVector2D force) {
            m_movingState->applyInertiaForce(vel, center, points, radius, force);
        },
        [this](const QPointF &pos) {
            m_physics.setLastWindowPos(pos);
        }
    );
}

void BlobAnimation::updatePhysics() {
    m_physics.updatePhysicsParallel(m_controlPoints, m_targetPoints, m_velocity,
                                    m_blobCenter, m_params, m_physicsParams);

    int padding = m_params.borderWidth + m_params.glowRadius;
    m_physics.handleBorderCollisions(m_controlPoints, m_velocity, m_blobCenter,
                                     width(), height(), m_physicsParams.restitution, padding);

    m_physics.smoothBlobShape(m_controlPoints);

    double minDistance = m_params.blobRadius * m_physicsParams.minNeighborDistance;
    double maxDistance = m_params.blobRadius * m_physicsParams.maxNeighborDistance;
    m_physics.constrainNeighborDistances(m_controlPoints, m_velocity, minDistance, maxDistance);
}


void BlobAnimation::resizeEvent(QResizeEvent *event) {
    // Deleguj obsługę zdarzenia resize do BlobEventHandler
    if (m_eventHandler.processResizeEvent(event)) {
        update();
    }
    QOpenGLWidget::resizeEvent(event);
}

void BlobAnimation::onStateResetTimeout() {
    if (m_currentState != BlobConfig::IDLE) {
        qDebug() << "Auto switching to IDLE state due to inactivity";
        switchToState(BlobConfig::IDLE);
    }
}

bool BlobAnimation::event(QEvent *event) {
    return QWidget::event(event);
}

void BlobAnimation::switchToState(BlobConfig::AnimationState newState) {
    // std::lock_guard<std::mutex> lock(m_pointsMutex);
    if (!m_eventsEnabled) {
        return;
    }

    if (m_currentState == newState) {
        if (newState == BlobConfig::MOVING || newState == BlobConfig::RESIZING) {
            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
        }
        return;
    }

    qDebug() << "State change from" << m_currentState << "to" << newState;

    // Zmieniona logika przejścia do Idle - bez animacji przejścia
    if (newState == BlobConfig::IDLE &&
        (m_currentState == BlobConfig::MOVING || m_currentState == BlobConfig::RESIZING)) {
        m_currentState = BlobConfig::IDLE;
        m_currentBlobState = m_idleState.get();

        // Reset inicjalizacji dla efektu bicia serca
        static_cast<IdleState *>(m_currentBlobState)->resetInitialization();

        // Wygaszamy prędkości, ale nie do zera - zostawiamy trochę dynamiki
        for (auto &vel: m_velocity) {
            vel *= 0.5;
        }

        return;
    }

    m_currentState = newState;

    switch (m_currentState) {
        case BlobConfig::IDLE:
            m_currentBlobState = m_idleState.get();
        // Reset inicjalizacji również tutaj
            static_cast<IdleState *>(m_currentBlobState)->resetInitialization();
            break;
        case BlobConfig::MOVING:
            m_currentBlobState = m_movingState.get();
            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
            break;
        case BlobConfig::RESIZING:
            m_currentBlobState = m_resizingState.get();
            m_stateResetTimer.stop();
            m_stateResetTimer.start(2000);
            break;
    }
}

void BlobAnimation::applyForces(const QVector2D &force) {
    m_currentBlobState->applyForce(force, m_velocity, m_blobCenter,
                                   m_controlPoints, m_params.blobRadius);
}

void BlobAnimation::applyIdleEffect() {
    m_idleState->apply(m_controlPoints, m_velocity, m_blobCenter, m_params);
}

void BlobAnimation::setBackgroundColor(const QColor &color) {
    qDebug() << "BlobAnimation::setBackgroundColor called with:" << color.name();
    if (m_params.backgroundColor != color) {
        m_params.backgroundColor = color;
        // Czy updateGridBuffer używa tego koloru? Czy trzeba wywołać coś jeszcze?
        m_renderer.updateGridBuffer(m_params.backgroundColor, m_params.gridColor, m_params.gridSpacing, width(), height());
        update(); // Wymuś przemalowanie
    }
}

void BlobAnimation::setGridColor(const QColor &color) {
    qDebug() << "BlobAnimation::setGridColor called with:" << color.name(QColor::HexArgb);
    if (m_params.gridColor != color) {
        m_params.gridColor = color;
        // Aktualizuj bufor siatki
        m_renderer.updateGridBuffer(m_params.backgroundColor, m_params.gridColor, m_params.gridSpacing, width(), height());
        update();
    }
}

void BlobAnimation::setGridSpacing(int spacing) {
    qDebug() << "BlobAnimation::setGridSpacing called with:" << spacing;
    if (m_params.gridSpacing != spacing && spacing > 0) {
        m_params.gridSpacing = spacing;
        // Aktualizuj bufor siatki
        m_renderer.updateGridBuffer(m_params.backgroundColor, m_params.gridColor, m_params.gridSpacing, width(), height());
        update();
    }
}

bool BlobAnimation::eventFilter(QObject *watched, QEvent *event) {
    // Deleguj filtrowanie zdarzeń do BlobEventHandler
    return m_eventHandler.eventFilter(watched, event) || QWidget::eventFilter(watched, event);
}

void BlobAnimation::setLifeColor(const QColor &color) {
    if (m_defaultLifeColor.isValid() == false) {
        m_defaultLifeColor = m_params.borderColor;
    }

    m_params.borderColor = color;

    m_needsRedraw = true;
    update();

    qDebug() << "Blob color changed to:" << color.name();
}

void BlobAnimation::resetLifeColor() {
    if (m_defaultLifeColor.isValid()) {
        m_params.borderColor = m_defaultLifeColor;

        m_needsRedraw = true;
        update();

        qDebug() << "Blob color reset to default:" << m_defaultLifeColor.name();
    }
}

void BlobAnimation::initializeGL() {
    // Inicjalizuj funkcje OpenGL
    initializeOpenGLFunctions();

    // Ustaw kolor tła
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Tworzenie programu shaderowego
    m_shaderProgram = new QOpenGLShaderProgram();

    // Vertex shader
    const char *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";

    // Fragment shader z efektem glow dla bloba
    const char *fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 blobColor;
        uniform vec2 blobCenter;
        uniform float blobRadius;

        void main() {
            // Oblicz odległość od środka bloba
            vec2 fragCoord = gl_FragCoord.xy;
            float distance = length(fragCoord - blobCenter);

            // Efekt glow
            float glow = 1.0 - smoothstep(blobRadius * 0.8, blobRadius, distance);
            FragColor = blobColor * glow;
        }
    )";

    // Kompiluj i linkuj shadery
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_shaderProgram->link();

    // Inicjalizuj VAO i VBO
    m_vao.create();
    m_vao.bind();

    m_vbo = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vbo.create();
    m_vbo.bind();

    // Ustaw atrybuty wierzchołków
    m_shaderProgram->enableAttributeArray(0);
    m_shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));

    m_vbo.release();
    m_vao.release();
}

void BlobAnimation::paintGL() {
    // Wyczyść bufor
    glClear(GL_COLOR_BUFFER_BIT);

    // Synchronizuj dostęp do punktów kontrolnych
    std::lock_guard<std::mutex> lock(m_pointsMutex);

    // Renderuj siatkę (możesz użyć własnej metody renderGrid)
    // drawGrid();

    // Konwertuj punkty kontrolne na wierzchołki OpenGL
    updateBlobGeometry();

    // Użyj programu shaderowego
    m_shaderProgram->bind();

    // Ustaw macierz projekcji
    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    m_shaderProgram->setUniformValue("projection", projection);

    // Ustaw parametry bloba
    QColor blobColor = m_params.borderColor;

    m_shaderProgram->setUniformValue("blobColor", QVector4D(
                                         blobColor.redF(), blobColor.greenF(), blobColor.blueF(), blobColor.alphaF()));
    m_shaderProgram->setUniformValue("blobCenter", QVector2D(m_blobCenter));
    m_shaderProgram->setUniformValue("blobRadius",
                                     static_cast<float>(m_params.blobRadius));

    // Renderuj bloba
    m_vao.bind();
    m_vbo.bind();
    m_vbo.allocate(m_glVertices.data(), m_glVertices.size() * sizeof(GLfloat));

    glDrawArrays(GL_TRIANGLE_FAN, 0, m_glVertices.size() / 2);

    m_vbo.release();
    m_vao.release();
    m_shaderProgram->release();
}

void BlobAnimation::resizeGL(int w, int h) {
    // Aktualizuj viewport
    glViewport(0, 0, w, h);
}

void BlobAnimation::updateBlobGeometry() {
    m_glVertices.clear();

    // Najpierw dodaj środek bloba (dla trybu TRIANGLE_FAN)
    m_glVertices.push_back(m_blobCenter.x());
    m_glVertices.push_back(m_blobCenter.y());

    // Dodaj wszystkie punkty kontrolne
    for (const auto &point: m_controlPoints) {
        m_glVertices.push_back(point.x());
        m_glVertices.push_back(point.y());
    }

    // Zamknij kształt, dodając pierwszy punkt ponownie
    if (!m_controlPoints.empty()) {
        m_glVertices.push_back(m_controlPoints[0].x());
        m_glVertices.push_back(m_controlPoints[0].y());
    }
}

void BlobAnimation::drawGrid() {
    // Upewnij się, że shader jest załadowany
    static QOpenGLShaderProgram *gridShader = nullptr;

    if (!gridShader) {
        gridShader = new QOpenGLShaderProgram(this);

        // Prosty shader dla siatki
        const char *gridVertexShader = R"(
            #version 330 core
            layout (location = 0) in vec2 position;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * vec4(position, 0.0, 1.0);
            }
        )";

        const char *gridFragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 gridColor;
            void main() {
                FragColor = gridColor;
            }
        )";

        gridShader->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShader);
        gridShader->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShader);
        gridShader->link();
    }

    // Użyj shadera dla siatki
    gridShader->bind();

    // Ustaw macierz projekcji
    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    gridShader->setUniformValue("projection", projection);

    // Ustaw kolor siatki
    QColor color = m_params.gridColor;
    gridShader->setUniformValue("gridColor", QVector4D(
                                   color.redF(), color.greenF(), color.blueF(), color.alphaF()));  // Usunięto mnożnik 0.5f

    // Przygotuj bufor dla linii siatki
    QOpenGLBuffer gridVBO(QOpenGLBuffer::VertexBuffer);
    gridVBO.create();
    gridVBO.bind();

    // Generuj wierzchołki dla linii siatki
    std::vector<GLfloat> gridVertices;
    int gridSpacing = m_params.gridSpacing;

    // Linie poziome
    for (int y = 0; y < height(); y += gridSpacing) {
        gridVertices.push_back(0);
        gridVertices.push_back(y);

        gridVertices.push_back(width());
        gridVertices.push_back(y);
    }

    // Linie pionowe
    for (int x = 0; x < width(); x += gridSpacing) {
        gridVertices.push_back(x);
        gridVertices.push_back(0);

        gridVertices.push_back(x);
        gridVertices.push_back(height());
    }

    // Załaduj dane do bufora
    gridVBO.allocate(gridVertices.data(), gridVertices.size() * sizeof(GLfloat));

    // Rysuj linie
    gridShader->enableAttributeArray(0);
    gridShader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));

    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, gridVertices.size() / 2);

    // Zwolnij zasoby
    gridShader->disableAttributeArray(0);
    gridVBO.release();
    gridShader->release();
    gridVBO.destroy();
}

void BlobAnimation::pauseAllEventTracking() {
    qDebug() << "Pauzowanie wszystkich mechanizmów śledzenia eventów";

    // Wyłącz flagę śledzenia eventów
    m_eventsEnabled = false;

    // Zatrzymaj wszystkie timery związane ze śledzeniem eventów
    m_windowPositionTimer.stop();
    m_stateResetTimer.stop();

    // Wyczyść wszystkie bufory ruchu
    m_transitionManager.clearAllMovementBuffers();

    // Poinformuj handler eventów o zatrzymaniu śledzenia
    m_eventHandler.disableEvents();

    // Upewnij się, że fizyka bloba jest w stabilnym stanie
    switchToState(BlobConfig::IDLE);
}

void BlobAnimation::resumeAllEventTracking() {
    qDebug() << "Wznawianie śledzenia eventów";

    // Zresetuj stan bloba
    initializeBlob();

    // Włącz flagę śledzenia eventów
    m_eventsEnabled = true;

    // Uruchom ponownie timery
    m_windowPositionTimer.start();

    // Poinformuj handler eventów o wznowieniu śledzenia
    m_eventHandler.enableEvents();

    // Wymuś reset hudu
    m_renderer.resetHUD();

    // Wymuś odświeżenie animacji
    switchToState(BlobConfig::IDLE);
    update();
}

void BlobAnimation::setBlobColor(const QColor &color) {
    qDebug() << "BlobAnimation::setBlobColor called with:" << color.name();
    // Załóżmy, że kolor bloba to 'borderColor' w parametrach
    if (m_params.borderColor != color) {
        m_params.borderColor = color;
        // Nie ma potrzeby aktualizować bufora siatki, tylko przemalować
        update(); // Wymuś przemalowanie (wywoła paintGL)
    }
}