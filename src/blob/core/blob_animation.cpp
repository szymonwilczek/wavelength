#include "blob_animation.h"

#include <QApplication>
#include <QDebug>
#include <QRandomGenerator>

#include "../../app/wavelength_config.h"

BlobAnimation::BlobAnimation(QWidget *parent)
    : QOpenGLWidget(parent),
      event_handler_(this),
      transition_manager_(this) {

    const WavelengthConfig *config = WavelengthConfig::GetInstance();

    params_.backgroundColor = config->GetBackgroundColor();
    params_.borderColor = config->GetBlobColor();
    params_.gridColor = config->GetGridColor();
    params_.gridSpacing = config->GetGridSpacing();

    params_.blobRadius = 250.0f;
    params_.numPoints = 32;
    params_.glowRadius = 10;
    params_.borderWidth = 3;

    idle_params_.waveAmplitude = 2.0;
    idle_params_.waveFrequency = 2.0;

    idle_state_ = std::make_unique<IdleState>();
    moving_state_ = std::make_unique<MovingState>();
    resizing_state_ = std::make_unique<ResizingState>();
    current_blob_state_ = idle_state_.get();

    control_points_.reserve(params_.numPoints);
    target_points_.reserve(params_.numPoints);
    velocity_.reserve(params_.numPoints);

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, true);

    InitializeBlob();

    physics_thread_ = std::thread(&BlobAnimation::PhysicsThreadFunction, this);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);

    resize_debounce_timer_.setSingleShot(true);
    resize_debounce_timer_.setInterval(50); // 50 ms debounce
    connect(&resize_debounce_timer_, &QTimer::timeout, this, &BlobAnimation::HandleResizeTimeout);

    if (window()) {
        last_window_position_ = window()->pos();
        last_window_position_timer_ = last_window_position_;
    }

    animation_timer_.setTimerType(Qt::PreciseTimer);
    animation_timer_.setInterval(16); // ~60 FPS
    connect(&animation_timer_, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    animation_timer_.start();

    window_position_timer_.setInterval(16); // (>100 FPS)
    window_position_timer_.setTimerType(Qt::PreciseTimer);
    connect(&window_position_timer_, &QTimer::timeout, this, &BlobAnimation::CheckWindowPosition);
    window_position_timer_.start();

    connect(&state_reset_timer_, &QTimer::timeout, this, &BlobAnimation::onStateResetTimeout);

    // Podłączenie eventów z BlobEventHandler
    connect(&event_handler_, &BlobEventHandler::windowMoved, this, [this](const QPointF &pos) {
        last_window_position_timer_ = pos;
    });

    connect(&event_handler_, &BlobEventHandler::movementSampleAdded, this,
            [this](const QPointF &pos, const qint64 timestamp) {
                transition_manager_.AddMovementSample(pos, timestamp);
            });

    connect(&event_handler_, &BlobEventHandler::resizeStateRequested, this, [this]() {
        SwitchToState(BlobConfig::RESIZING);
    });

    connect(&event_handler_, &BlobEventHandler::significantResizeDetected, this,
            [this](const QSize &oldSize, const QSize &newSize) {
                resizing_state_->handleResize(control_points_, target_points_, velocity_,
                                              blob_center_, oldSize, newSize);

                // Resetuj bufor siatki tylko gdy zmiana rozmiaru jest znacząca
                if (abs(newSize.width() - last_size_.width()) > 20 ||
                    abs(newSize.height() - last_size_.height()) > 20) {
                    renderer_.resetGridBuffer();
                }

                last_size_ = newSize;
            });

    connect(&event_handler_, &BlobEventHandler::stateResetTimerRequested, this, [this] {
        state_reset_timer_.stop();
        state_reset_timer_.start(2000);
    });

    connect(&event_handler_, &BlobEventHandler::eventsReEnabled, this, [this] {
        events_enabled_ = true;
        qDebug() << "Events re-enabled via handler";
    });

    connect(&transition_manager_, &BlobTransitionManager::transitionCompleted, this, [this] {
        event_re_enable_timer_.start(200);
    });

    connect(&transition_manager_, &BlobTransitionManager::significantMovementDetected, this, [this] {
        SwitchToState(BlobConfig::MOVING);
        needs_redraw_ = true;
    });

    connect(&transition_manager_, &BlobTransitionManager::movementStopped, this, [this] {
        if (current_state_ == BlobConfig::MOVING) {
            SwitchToState(BlobConfig::IDLE);
        }
    });

    event_re_enable_timer_.setSingleShot(true);
    connect(&event_re_enable_timer_, &QTimer::timeout, this, [this] {
        events_enabled_ = true;
        event_handler_.EnableEvents();
        qDebug() << "Events re-enabled";
    });
}

void BlobAnimation::HandleResizeTimeout() {
    // Po każdym zdarzeniu resize, resetujemy bloba do oryginalnego rozmiaru i środka
    ResetBlobToCenter();

    // Resetujemy bufor siatki
    renderer_.resetGridBuffer();
    last_size_ = size();

    // Odświeżamy widok
    update();
}

void BlobAnimation::CheckWindowPosition() {
    const QWidget *currentWindow = window();
    if (!events_enabled_ || !currentWindow)
        return;

    static qint64 last_check_time = 0;
    const qint64 current_timestamp = QDateTime::currentMSecsSinceEpoch();

    if (current_timestamp - last_check_time < 16) {
        return;
    }

    last_check_time = current_timestamp;

    std::pmr::deque<BlobTransitionManager::WindowMovementSample> movementBuffer = transition_manager_.
            GetMovementBuffer();

    if (movementBuffer.empty() ||
        (current_timestamp - transition_manager_.GetLastMovementTime()) > 100) {
        if (const QPointF currentWindowPos = currentWindow->pos(); movementBuffer.empty() || currentWindowPos != movementBuffer.back().position) {
            transition_manager_.AddMovementSample(currentWindowPos, current_timestamp);
        }
    }
}

void BlobAnimation::PhysicsThreadFunction() {
    while (physics_active_) {
        auto start_time = std::chrono::high_resolution_clock::now(); {
            std::lock_guard lock(points_mutex_);
            updatePhysics();
        }

        // Poinformuj wątek UI o konieczności odświeżenia
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

        // Utrzymuj stałą liczbę klatek
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

        if (auto target_frame_time = std::chrono::microseconds(8333); duration < target_frame_time) {
            std::unique_lock lock(points_mutex_);
            physics_condition_.wait_for(lock, target_frame_time - duration);
        }
    }
}

BlobAnimation::~BlobAnimation() {
    physics_active_ = false;
    physics_condition_.notify_all();
    if (physics_thread_.joinable()) {
        physics_thread_.join();
    }

    makeCurrent();
    vbo_.destroy();
    vao_.destroy();
    delete shader_program_;
    doneCurrent();

    animation_timer_.stop();
    idle_timer_.stop();
    state_reset_timer_.stop();

    idle_state_.reset();
    moving_state_.reset();
    resizing_state_.reset();
}

void BlobAnimation::InitializeBlob() {
    std::lock_guard lock(points_mutex_);
    // Wyczyść aktualne punkty
    control_points_.clear();
    target_points_.clear();
    velocity_.clear();

    // Ustaw bloba na środek ekranu
    blob_center_ = QPointF(width() / 2.0, height() / 2.0);

    // Generuj punkty kontrolne w nieregularnym, organicznym kształcie
    control_points_ = GenerateOrganicShape(blob_center_, params_.blobRadius, params_.numPoints);

    // Ustawiamy punkty docelowe i prędkości
    target_points_ = control_points_;
    velocity_.resize(params_.numPoints, QPointF(0, 0));

    // To zapewni, że nawet jeśli parametry fizyki będą próbować
    // zmienić rozmiar, to my go zawsze resetujemy do określonej wartości
    precalc_min_distance_ = params_.blobRadius * physics_params_.minNeighborDistance;
    precalc_max_distance_ = params_.blobRadius * physics_params_.maxNeighborDistance;
}

void BlobAnimation::paintEvent(QPaintEvent *event) {
    std::lock_guard lock(points_mutex_);

    if (const QRectF blob_rect = CalculateBlobBoundingRect(); !event->rect().intersects(blob_rect.toRect())) {
        return;
    }

    static QPixmap background_cache;
    static QSize last_background_size;
    static QColor last_background_color;
    static QColor last_grid_color;
    static int last_grid_spacing;

    QPainter painter(this);

    // Przygotuj stan renderowania
    BlobRenderState blob_render_state;
    blob_render_state.animationState = current_state_;

    // Deleguj renderowanie do BlobRenderer
    renderer_.renderScene(
        painter,
        control_points_,
        blob_center_,
        params_,
        blob_render_state,
        width(),
        height(),
        background_cache,
        last_background_size,
        last_background_color,
        last_grid_color,
        last_grid_spacing
    );
}

QRectF BlobAnimation::CalculateBlobBoundingRect() {
    if (control_points_.empty()) {
        return QRectF(0, 0, width(), height());
    }

    auto x_comp = [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); };
    auto y_comp = [](const QPointF &a, const QPointF &b) { return a.y() < b.y(); };

    auto [minXIt, maxXIt] = std::minmax_element(control_points_.begin(), control_points_.end(), x_comp);
    auto [minYIt, maxYIt] = std::minmax_element(control_points_.begin(), control_points_.end(), y_comp);

    const qreal min_x = minXIt->x();
    const qreal max_x = maxXIt->x();
    const qreal min_y = minYIt->y();
    const qreal max_y = maxYIt->y();

    const int margin = params_.borderWidth + params_.glowRadius + 5;
    return QRectF(min_x - margin, min_y - margin,
                  max_x - min_x + 2 * margin, max_y - min_y + 2 * margin);
}


void BlobAnimation::updateAnimation() {
    needs_redraw_ = false;

    { // <<< Dodaj zakres dla blokady
        std::lock_guard lock(points_mutex_); // <<< DODAJ BLOKADĘ

        // processMovementBuffer może modyfikować wektory przez applyInertiaForce
        processMovementBuffer();

        // Stany IDLE/MOVING/RESIZING mogą modyfikować wektory w metodzie apply
        if (current_state_ == BlobConfig::IDLE) {
            if (current_blob_state_) {
                current_blob_state_->apply(control_points_, velocity_, blob_center_, params_);
            }
        } else if (current_state_ == BlobConfig::MOVING || current_state_ == BlobConfig::RESIZING) {
            if (current_blob_state_) {
                current_blob_state_->apply(control_points_, velocity_, blob_center_, params_);
                needs_redraw_ = true;
            }
        }
    } // <<< Blokada zwalniana automatycznie

    if (needs_redraw_) {
        update();
        params_.screenWidth = width();
        params_.screenHeight = height();
    }
}

void BlobAnimation::processMovementBuffer() {
    transition_manager_.ProcessMovementBuffer(
        velocity_,
        blob_center_,
        control_points_,
        params_.blobRadius,
        [this](std::vector<QPointF> &vel, QPointF &center, const std::vector<QPointF> &points, const float radius,
               const QVector2D force) {
            moving_state_->applyInertiaForce(vel, center, points, radius, force);
        },
        [this](const QPointF &pos) {
            physics_.setLastWindowPos(pos);
        }
    );
}

void BlobAnimation::updatePhysics() {
    physics_.updatePhysicsParallel(control_points_, target_points_, velocity_,
                                    blob_center_, params_, physics_params_);

    const int padding = params_.borderWidth + params_.glowRadius;
    physics_.handleBorderCollisions(control_points_, velocity_, blob_center_,
                                     width(), height(), physics_params_.restitution, padding);

    physics_.smoothBlobShape(control_points_);

    const double min_distance = params_.blobRadius * physics_params_.minNeighborDistance;
    const double max_distance = params_.blobRadius * physics_params_.maxNeighborDistance;
    physics_.constrainNeighborDistances(control_points_, velocity_, min_distance, max_distance);
}


void BlobAnimation::resizeEvent(QResizeEvent *event) {
    // Deleguj obsługę zdarzenia resize do BlobEventHandler
    if (event_handler_.ProcessResizeEvent(event)) {
        update();
    }
    QOpenGLWidget::resizeEvent(event);
}

void BlobAnimation::onStateResetTimeout() {
    if (current_state_ != BlobConfig::IDLE) {
        qDebug() << "Auto switching to IDLE state due to inactivity";
        SwitchToState(BlobConfig::IDLE);
    }
}

bool BlobAnimation::event(QEvent *event) {
    return QWidget::event(event);
}

void BlobAnimation::SwitchToState(BlobConfig::AnimationState new_state) {
    // std::lock_guard<std::mutex> lock(m_pointsMutex);
    if (!events_enabled_) {
        return;
    }

    if (current_state_ == new_state) {
        if (new_state == BlobConfig::MOVING || new_state == BlobConfig::RESIZING) {
            state_reset_timer_.stop();
            state_reset_timer_.start(2000);
        }
        return;
    }

    qDebug() << "State change from" << current_state_ << "to" << new_state;

    // Zmieniona logika przejścia do Idle - bez animacji przejścia
    if (new_state == BlobConfig::IDLE &&
        (current_state_ == BlobConfig::MOVING || current_state_ == BlobConfig::RESIZING)) {
        current_state_ = BlobConfig::IDLE;
        current_blob_state_ = idle_state_.get();

        // Reset inicjalizacji dla efektu bicia serca
        static_cast<IdleState *>(current_blob_state_)->resetInitialization();

        // Wygaszamy prędkości, ale nie do zera - zostawiamy trochę dynamiki
        for (auto &vel: velocity_) {
            vel *= 0.5;
        }

        return;
    }

    current_state_ = new_state;

    switch (current_state_) {
        case BlobConfig::IDLE:
            current_blob_state_ = idle_state_.get();
        // Reset inicjalizacji również tutaj
            static_cast<IdleState *>(current_blob_state_)->resetInitialization();
            break;
        case BlobConfig::MOVING:
            current_blob_state_ = moving_state_.get();
            state_reset_timer_.stop();
            state_reset_timer_.start(2000);
            break;
        case BlobConfig::RESIZING:
            current_blob_state_ = resizing_state_.get();
            state_reset_timer_.stop();
            state_reset_timer_.start(2000);
            break;
    }
}

void BlobAnimation::ApplyForces(const QVector2D &force) {
    current_blob_state_->applyForce(force, velocity_, blob_center_,
                                   control_points_, params_.blobRadius);
}

void BlobAnimation::ApplyIdleEffect() {
    idle_state_->apply(control_points_, velocity_, blob_center_, params_);
}

void BlobAnimation::setBackgroundColor(const QColor &color) {
    qDebug() << "BlobAnimation::setBackgroundColor called with:" << color.name();
    if (params_.backgroundColor != color) {
        params_.backgroundColor = color;
        // Czy updateGridBuffer używa tego koloru? Czy trzeba wywołać coś jeszcze?
        renderer_.updateGridBuffer(params_.backgroundColor, params_.gridColor, params_.gridSpacing, width(), height());
        update(); // Wymuś przemalowanie
    }
}

void BlobAnimation::setGridColor(const QColor &color) {
    qDebug() << "BlobAnimation::setGridColor called with:" << color.name(QColor::HexArgb);
    if (params_.gridColor != color) {
        params_.gridColor = color;
        // Aktualizuj bufor siatki
        renderer_.updateGridBuffer(params_.backgroundColor, params_.gridColor, params_.gridSpacing, width(), height());
        update();
    }
}

void BlobAnimation::setGridSpacing(const int spacing) {
    qDebug() << "BlobAnimation::setGridSpacing called with:" << spacing;
    if (params_.gridSpacing != spacing && spacing > 0) {
        params_.gridSpacing = spacing;
        // Aktualizuj bufor siatki
        renderer_.updateGridBuffer(params_.backgroundColor, params_.gridColor, params_.gridSpacing, width(), height());
        update();
    }
}

bool BlobAnimation::eventFilter(QObject *watched, QEvent *event) {
    // Deleguj filtrowanie zdarzeń do BlobEventHandler
    return event_handler_.eventFilter(watched, event) || QWidget::eventFilter(watched, event);
}

void BlobAnimation::SetLifeColor(const QColor &color) {
    if (default_life_color_.isValid() == false) {
        default_life_color_ = params_.borderColor;
    }

    params_.borderColor = color;

    needs_redraw_ = true;
    update();

    qDebug() << "Blob color changed to:" << color.name();
}

void BlobAnimation::ResetLifeColor() {
    if (default_life_color_.isValid()) {
        params_.borderColor = default_life_color_;

        needs_redraw_ = true;
        update();

        qDebug() << "Blob color reset to default:" << default_life_color_.name();
    }
}

void BlobAnimation::initializeGL() {
    // Inicjalizuj funkcje OpenGL
    initializeOpenGLFunctions();

    // Ustaw kolor tła
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Tworzenie programu shaderowego
    shader_program_ = new QOpenGLShaderProgram();

    // Vertex shader
    const auto vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";

    // Fragment shader z efektem glow dla bloba
    const auto fragment_shader_source = R"(
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
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source);
    shader_program_->link();

    // Inicjalizuj VAO i VBO
    vao_.create();
    vao_.bind();

    vbo_ = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo_.create();
    vbo_.bind();

    // Ustaw atrybuty wierzchołków
    shader_program_->enableAttributeArray(0);
    shader_program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));

    vbo_.release();
    vao_.release();
}

void BlobAnimation::paintGL() {
    // Wyczyść bufor
    glClear(GL_COLOR_BUFFER_BIT);

    // Synchronizuj dostęp do punktów kontrolnych
    std::lock_guard lock(points_mutex_);

    // Renderuj siatkę (możesz użyć własnej metody renderGrid)
    // drawGrid();

    // Konwertuj punkty kontrolne na wierzchołki OpenGL
    UpdateBlobGeometry();

    // Użyj programu shaderowego
    shader_program_->bind();

    // Ustaw macierz projekcji
    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    shader_program_->setUniformValue("projection", projection);

    // Ustaw parametry bloba
    const QColor blob_color = params_.borderColor;

    shader_program_->setUniformValue("blobColor", QVector4D(
                                         blob_color.redF(), blob_color.greenF(), blob_color.blueF(), blob_color.alphaF()));
    shader_program_->setUniformValue("blobCenter", QVector2D(blob_center_));
    shader_program_->setUniformValue("blobRadius",
                                     static_cast<float>(params_.blobRadius));

    // Renderuj bloba
    vao_.bind();
    vbo_.bind();
    vbo_.allocate(gl_vertices_.data(), gl_vertices_.size() * sizeof(GLfloat));

    glDrawArrays(GL_TRIANGLE_FAN, 0, gl_vertices_.size() / 2);

    vbo_.release();
    vao_.release();
    shader_program_->release();
}

void BlobAnimation::resizeGL(int w, int h) {
    // Aktualizuj viewport
    glViewport(0, 0, w, h);
}

void BlobAnimation::UpdateBlobGeometry() {
    gl_vertices_.clear();

    // Najpierw dodaj środek bloba (dla trybu TRIANGLE_FAN)
    gl_vertices_.push_back(blob_center_.x());
    gl_vertices_.push_back(blob_center_.y());

    // Dodaj wszystkie punkty kontrolne
    for (const auto &point: control_points_) {
        gl_vertices_.push_back(point.x());
        gl_vertices_.push_back(point.y());
    }

    // Zamknij kształt, dodając pierwszy punkt ponownie
    if (!control_points_.empty()) {
        gl_vertices_.push_back(control_points_[0].x());
        gl_vertices_.push_back(control_points_[0].y());
    }
}

void BlobAnimation::DrawGrid() {
    // Upewnij się, że shader jest załadowany
    static QOpenGLShaderProgram *grid_shader = nullptr;

    if (!grid_shader) {
        grid_shader = new QOpenGLShaderProgram(this);

        // Prosty shader dla siatki
        const auto grid_vertex_shader = R"(
            #version 330 core
            layout (location = 0) in vec2 position;
            uniform mat4 projection;
            void main() {
                gl_Position = projection * vec4(position, 0.0, 1.0);
            }
        )";

        const auto grid_fragment_shader = R"(
            #version 330 core
            out vec4 FragColor;
            uniform vec4 gridColor;
            void main() {
                FragColor = gridColor;
            }
        )";

        grid_shader->addShaderFromSourceCode(QOpenGLShader::Vertex, grid_vertex_shader);
        grid_shader->addShaderFromSourceCode(QOpenGLShader::Fragment, grid_fragment_shader);
        grid_shader->link();
    }

    // Użyj shadera dla siatki
    grid_shader->bind();

    // Ustaw macierz projekcji
    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    grid_shader->setUniformValue("projection", projection);

    // Ustaw kolor siatki
    const QColor grid_color = params_.gridColor;
    grid_shader->setUniformValue("gridColor", QVector4D(
                                   grid_color.redF(), grid_color.greenF(), grid_color.blueF(), grid_color.alphaF()));  // Usunięto mnożnik 0.5f

    // Przygotuj bufor dla linii siatki
    QOpenGLBuffer grid_vbo(QOpenGLBuffer::VertexBuffer);
    grid_vbo.create();
    grid_vbo.bind();

    // Generuj wierzchołki dla linii siatki
    std::vector<GLfloat> grid_vertices;
    const int grid_spacing = params_.gridSpacing;

    // Linie poziome
    for (int y = 0; y < height(); y += grid_spacing) {
        grid_vertices.push_back(0);
        grid_vertices.push_back(y);

        grid_vertices.push_back(width());
        grid_vertices.push_back(y);
    }

    // Linie pionowe
    for (int x = 0; x < width(); x += grid_spacing) {
        grid_vertices.push_back(x);
        grid_vertices.push_back(0);

        grid_vertices.push_back(x);
        grid_vertices.push_back(height());
    }

    // Załaduj dane do bufora
    grid_vbo.allocate(grid_vertices.data(), grid_vertices.size() * sizeof(GLfloat));

    // Rysuj linie
    grid_shader->enableAttributeArray(0);
    grid_shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));

    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, grid_vertices.size() / 2);

    // Zwolnij zasoby
    grid_shader->disableAttributeArray(0);
    grid_vbo.release();
    grid_shader->release();
    grid_vbo.destroy();
}

void BlobAnimation::PauseAllEventTracking() {
    qDebug() << "Pauzowanie wszystkich mechanizmów śledzenia eventów";

    // Wyłącz flagę śledzenia eventów
    events_enabled_ = false;

    // Zatrzymaj wszystkie timery związane ze śledzeniem eventów
    window_position_timer_.stop();
    state_reset_timer_.stop();

    // Wyczyść wszystkie bufory ruchu
    transition_manager_.ClearAllMovementBuffers();

    // Poinformuj handler eventów o zatrzymaniu śledzenia
    event_handler_.DisableEvents();

    // Upewnij się, że fizyka bloba jest w stabilnym stanie
    SwitchToState(BlobConfig::IDLE);
}

void BlobAnimation::ResumeAllEventTracking() {
    qDebug() << "Wznawianie śledzenia eventów";

    // Zresetuj stan bloba
    InitializeBlob();

    // Włącz flagę śledzenia eventów
    events_enabled_ = true;

    // Uruchom ponownie timery
    window_position_timer_.start();

    // Poinformuj handler eventów o wznowieniu śledzenia
    event_handler_.EnableEvents();

    // Wymuś reset hudu
    renderer_.resetHUD();

    // Wymuś odświeżenie animacji
    SwitchToState(BlobConfig::IDLE);
    update();
}

void BlobAnimation::setBlobColor(const QColor &color) {
    qDebug() << "BlobAnimation::setBlobColor called with:" << color.name();
    // Załóżmy, że kolor bloba to 'borderColor' w parametrach
    if (params_.borderColor != color) {
        params_.borderColor = color;
        // Nie ma potrzeby aktualizować bufora siatki, tylko przemalować
        update(); // Wymuś przemalowanie (wywoła paintGL)
    }
}

std::vector<QPointF> BlobAnimation::GenerateOrganicShape(const QPointF& center, const double base_radius, const int num_of_points) {
    std::vector<QPointF> points;
    points.reserve(num_of_points);

    std::vector random_factors(num_of_points, 1.0);

    // Komponent losowych zakłóceń (wysokiej częstotliwości)
    for (int i = 0; i < num_of_points; ++i) {
        constexpr double max_deformation = 1.1;
        constexpr double min_deformaton = 0.9;
        // Mniejszy zakres losowości
        const double base_factor = min_deformaton + QRandomGenerator::global()->generateDouble() * (max_deformation - min_deformaton);
        random_factors[i] = base_factor;
    }

    // Dodajemy komponenty o niższych częstotliwościach - ale z mniejszą amplitudą
    const int lobes = 2 + QRandomGenerator::global()->bounded(3); // 2-4 główne płaty
    const double lobe_phase = QRandomGenerator::global()->generateDouble() * M_PI * 2; // Losowe przesunięcie fazy

    for (int i = 0; i < num_of_points; ++i) {
        const double angle = 2.0 * M_PI * i / num_of_points;
        // Mniejsze falowanie dla głównych płatów
        const double lobe_factor = 0.07 * sin(angle * lobes + lobe_phase);
        // Mniejsze falowanie dla dodatkowych nieregularności
        const double mid_factor = 0.04 * sin(angle * (lobes*2+1) + lobe_phase * 1.5);

        // Łączymy wszystkie komponenty
        random_factors[i] = random_factors[i] + lobe_factor + mid_factor;

        // Węższy dopuszczalny zakres
        random_factors[i] = qBound(0.85, random_factors[i], 1.15);
    }

    // Więcej przejść wygładzania dla bardziej płynnego kształtu
    std::vector<double> smoothed_factors = random_factors;
    for (int smooth_pass = 0; smooth_pass < 3; ++smooth_pass) {
        std::vector<double> temp_factors = smoothed_factors;
        for (int i = 0; i < num_of_points; ++i) {
            const int prev = i > 0 ? i - 1 : num_of_points - 1;
            const int next = i < num_of_points - 1 ? i + 1 : 0;
            // Większa waga dla centralnego punktu = większa regularność
            smoothed_factors[i] = (temp_factors[prev] * 0.2 +
                                  temp_factors[i] * 0.6 +
                                  temp_factors[next] * 0.2);
        }
    }

    // Dodatkowe sprawdzenie maksymalnego promienia
    const double max_allowed_radius = base_radius * 0.98; // Pozostawiamy 2% marginesu dla bezpieczeństwa

    // Generujemy punkty finalne
    for (int i = 0; i < num_of_points; ++i) {
        const double angle = 2.0 * M_PI * i / num_of_points;
        // Skalowanie promienia aby nigdy nie przekroczyć bazowego promienia
        double deformed_radius = base_radius * smoothed_factors[i];
        if (deformed_radius > max_allowed_radius) {
            deformed_radius = max_allowed_radius;
        }

        points.push_back(QPointF(
            center.x() + deformed_radius * cos(angle),
            center.y() + deformed_radius * sin(angle)
        ));
    }

    return points;
}

void BlobAnimation::ResetBlobToCenter() {
    std::lock_guard lock(points_mutex_);
    // Zapamiętaj aktualny promień bloba
    const double original_radius = params_.blobRadius;

    // Całkowicie zresetuj bloba
    control_points_.clear();
    target_points_.clear();
    velocity_.clear();

    // Ustaw bloba dokładnie na środku
    blob_center_ = QPointF(width() / 2.0, height() / 2.0);

    // Generuj punkty kontrolne w nieregularnym, organicznym kształcie
    control_points_ = GenerateOrganicShape(blob_center_, original_radius, params_.numPoints);
    target_points_ = control_points_;
    velocity_.resize(params_.numPoints, QPointF(0, 0));

    // Przełącz na stan IDLE i zresetuj zachowanie
    if (idle_state_) {
        idle_state_.get()->resetInitialization();
    }
    SwitchToState(BlobConfig::IDLE);
}

void BlobAnimation::ResetVisualization() {
    // Całkowicie resetujemy bloba do stanu początkowego
    // z oryginalnym rozmiarem i pozycją na środku
    ResetBlobToCenter();

    // Wymuś reset i reinicjalizację HUD
    renderer_.resetHUD();
    renderer_.forceHUDInitialization(blob_center_, params_.blobRadius,
                                    params_.borderColor, width(), height());

    // Sygnał informujący o konieczności aktualizacji innych elementów UI
    emit visualizationReset();

    // Odśwież widok
    update();
}

void BlobAnimation::showAnimation() {
    if (!isVisible()) {
        setVisible(true);
        ResumeAllEventTracking();
    }
}

void BlobAnimation::hideAnimation() {
    if (isVisible()) {
        setVisible(false);
        PauseAllEventTracking();
    }
}
