#include "blob_animation.h"

#include <QDateTime>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QPaintEvent>
#include <QRandomGenerator>

#include "../../app/wavelength_config.h"
#include "../states/idle_state.h"
#include "../states/moving_state.h"
#include "../states/resizing_state.h"

BlobAnimation::BlobAnimation(QWidget *parent)
    : QOpenGLWidget(parent),
      event_handler_(this),
      transition_manager_(this) {
    const WavelengthConfig *config = WavelengthConfig::GetInstance();

    params_.background_color = config->GetBackgroundColor();
    params_.border_color = config->GetBlobColor();
    params_.grid_color = config->GetGridColor();
    params_.grid_spacing = config->GetGridSpacing();

    params_.blob_radius = 250.0f;
    params_.num_of_points = 32;
    params_.glow_radius = 10;
    params_.border_width = 3;

    idle_params_.wave_amplitude = 2.0;
    idle_params_.wave_frequency = 2.0;

    idle_state_ = std::make_unique<IdleState>();
    moving_state_ = std::make_unique<MovingState>();
    resizing_state_ = std::make_unique<ResizingState>();
    current_blob_state_ = idle_state_.get();

    control_points_.reserve(params_.num_of_points);
    target_points_.reserve(params_.num_of_points);
    velocity_.reserve(params_.num_of_points);

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
    resize_debounce_timer_.setInterval(50);
    connect(&resize_debounce_timer_, &QTimer::timeout, this, &BlobAnimation::HandleResizeTimeout);

    if (window()) {
        last_window_position_ = window()->pos();
        last_window_position_timer_ = last_window_position_;
    }

    animation_timer_.setTimerType(Qt::PreciseTimer);
    animation_timer_.setInterval(16); // ~60 FPS
    connect(&animation_timer_, &QTimer::timeout, this, &BlobAnimation::updateAnimation);
    animation_timer_.start();

    window_position_timer_.setInterval(16);
    window_position_timer_.setTimerType(Qt::PreciseTimer);
    connect(&window_position_timer_, &QTimer::timeout, this, &BlobAnimation::CheckWindowPosition);
    window_position_timer_.start();

    connect(&state_reset_timer_, &QTimer::timeout, this, &BlobAnimation::onStateResetTimeout);

    // window moving events connection
    connect(&event_handler_, &BlobEventHandler::windowMoved, this, [this](const QPointF &pos) {
        last_window_position_timer_ = pos;
    });

    // window resize events connections
    connect(&event_handler_, &BlobEventHandler::movementSampleAdded, this,
            [this](const QPointF &pos, const qint64 timestamp) {
                transition_manager_.AddMovementSample(pos, timestamp);
            });
    connect(&event_handler_, &BlobEventHandler::resizeStateRequested, this, [this] {
        SwitchToState(BlobConfig::kResizing);
    });
    connect(&event_handler_, &BlobEventHandler::significantResizeDetected, this,
            [this](const QSize &oldSize, const QSize &newSize) {
                resizing_state_->HandleResize(control_points_, target_points_, velocity_,
                                              blob_center_, oldSize, newSize);
                if (abs(newSize.width() - last_size_.width()) > 20 ||
                    abs(newSize.height() - last_size_.height()) > 20) {
                    renderer_.ResetGridBuffer();
                }
                last_size_ = newSize;
            });

    // states connections
    connect(&event_handler_, &BlobEventHandler::stateResetTimerRequested, this, [this] {
        state_reset_timer_.stop();
        state_reset_timer_.start(2000);
    });
    connect(&event_handler_, &BlobEventHandler::eventsReEnabled, this, [this] {
        events_enabled_ = true;
    });
    connect(&transition_manager_, &BlobTransitionManager::transitionCompleted, this, [this] {
        event_re_enable_timer_.start(200);
    });
    connect(&transition_manager_, &BlobTransitionManager::significantMovementDetected, this, [this] {
        SwitchToState(BlobConfig::kMoving);
        needs_redraw_ = true;
    });
    connect(&transition_manager_, &BlobTransitionManager::movementStopped, this, [this] {
        if (current_state_ == BlobConfig::kMoving) {
            SwitchToState(BlobConfig::kIdle);
        }
    });

    // event re-enable timer
    event_re_enable_timer_.setSingleShot(true);
    connect(&event_re_enable_timer_, &QTimer::timeout, this, [this] {
        events_enabled_ = true;
        event_handler_.EnableEvents();
    });
}

void BlobAnimation::HandleResizeTimeout() {
    ResetBlobToCenter();
    renderer_.ResetGridBuffer();
    last_size_ = size();
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
    const std::pmr::deque<BlobTransitionManager::WindowMovementSample> movementBuffer = transition_manager_.
            GetMovementBuffer();

    if (movementBuffer.empty() ||
        current_timestamp - transition_manager_.GetLastMovementTime() > 100) {
        if (const QPointF currentWindowPos = currentWindow->pos();
            movementBuffer.empty() || currentWindowPos != movementBuffer.back().position) {
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

        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

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
    state_reset_timer_.stop();

    idle_state_.reset();
    moving_state_.reset();
    resizing_state_.reset();
}

void BlobAnimation::InitializeBlob() {
    std::lock_guard lock(points_mutex_);
    control_points_.clear();
    target_points_.clear();
    velocity_.clear();

    blob_center_ = QPointF(width() / 2.0, height() / 2.0);
    control_points_ = GenerateOrganicShape(blob_center_, params_.blob_radius, params_.num_of_points);
    target_points_ = control_points_;
    velocity_.resize(params_.num_of_points, QPointF(0, 0));

    precalc_min_distance_ = params_.blob_radius * physics_params_.min_neighbor_distance;
    precalc_max_distance_ = params_.blob_radius * physics_params_.max_neighbor_distance;
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

    BlobRenderState blob_render_state;
    blob_render_state.animation_state = current_state_;

    renderer_.RenderScene(
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
        return {0, 0, static_cast<qreal>(width()), static_cast<qreal>(height())};
    }

    auto x_comp = [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); };
    auto y_comp = [](const QPointF &a, const QPointF &b) { return a.y() < b.y(); };

    auto [minXIt, maxXIt] = std::minmax_element(control_points_.begin(), control_points_.end(), x_comp);
    auto [minYIt, maxYIt] = std::minmax_element(control_points_.begin(), control_points_.end(), y_comp);

    const qreal min_x = minXIt->x();
    const qreal max_x = maxXIt->x();
    const qreal min_y = minYIt->y();
    const qreal max_y = maxYIt->y();

    const int margin = params_.border_width + params_.glow_radius + 5;
    return {
        min_x - margin, min_y - margin,
        max_x - min_x + 2 * margin, max_y - min_y + 2 * margin
    };
}


void BlobAnimation::updateAnimation() {
    needs_redraw_ = false; {
        std::lock_guard lock(points_mutex_);

        processMovementBuffer();

        if (current_state_ == BlobConfig::kIdle) {
            if (current_blob_state_) {
                current_blob_state_->Apply(control_points_, velocity_, blob_center_, params_);
            }
        } else if (current_state_ == BlobConfig::kMoving || current_state_ == BlobConfig::kResizing) {
            if (current_blob_state_) {
                current_blob_state_->Apply(control_points_, velocity_, blob_center_, params_);
                needs_redraw_ = true;
            }
        }
    }

    if (needs_redraw_) {
        update();
        params_.screen_width = width();
        params_.screen_height = height();
    }
}

void BlobAnimation::processMovementBuffer() {
    transition_manager_.ProcessMovementBuffer(
        velocity_,
        blob_center_,
        control_points_,
        params_.blob_radius,
        [this](std::vector<QPointF> &vel, QPointF &center, const std::vector<QPointF> &points, const float radius,
               const QVector2D force) {
            MovingState::ApplyInertiaForce(vel, center, points, radius, force);
        },
        [this](const QPointF &pos) {
            physics_.SetLastWindowPos(pos);
        }
    );
}

void BlobAnimation::updatePhysics() {
    physics_.UpdatePhysicsParallel(control_points_, target_points_, velocity_,
                                   blob_center_, params_, physics_params_);

    const int padding = params_.border_width + params_.glow_radius;
    BlobPhysics::HandleBorderCollisions(control_points_, velocity_, blob_center_,
                                        width(), height(), physics_params_.restitution, padding);

    BlobPhysics::SmoothBlobShape(control_points_);

    const double min_distance = params_.blob_radius * physics_params_.min_neighbor_distance;
    const double max_distance = params_.blob_radius * physics_params_.max_neighbor_distance;
    BlobPhysics::ConstrainNeighborDistances(control_points_, velocity_, min_distance, max_distance);
}


void BlobAnimation::resizeEvent(QResizeEvent *event) {
    if (event_handler_.ProcessResizeEvent(event)) {
        update();
    }
    QOpenGLWidget::resizeEvent(event);
}

void BlobAnimation::onStateResetTimeout() {
    if (current_state_ != BlobConfig::kIdle) {
        SwitchToState(BlobConfig::kIdle);
    }
}

bool BlobAnimation::event(QEvent *event) {
    return QOpenGLWidget::event(event);
}

void BlobAnimation::SwitchToState(const BlobConfig::AnimationState new_state) {
    if (!events_enabled_) {
        return;
    }

    if (current_state_ == new_state) {
        if (new_state == BlobConfig::kMoving || new_state == BlobConfig::kResizing) {
            state_reset_timer_.stop();
            state_reset_timer_.start(2000);
        }
        return;
    }

    if (new_state == BlobConfig::kIdle &&
        (current_state_ == BlobConfig::kMoving || current_state_ == BlobConfig::kResizing)) {
        current_state_ = BlobConfig::kIdle;
        current_blob_state_ = idle_state_.get();

        dynamic_cast<IdleState *>(current_blob_state_)->ResetInitialization();

        for (auto &vel: velocity_) {
            vel *= 0.5;
        }

        return;
    }

    current_state_ = new_state;

    switch (current_state_) {
        case BlobConfig::kIdle:
            current_blob_state_ = idle_state_.get();
            dynamic_cast<IdleState *>(current_blob_state_)->ResetInitialization();
            break;
        case BlobConfig::kMoving:
            current_blob_state_ = moving_state_.get();
            state_reset_timer_.stop();
            state_reset_timer_.start(2000);
            break;
        case BlobConfig::kResizing:
            current_blob_state_ = resizing_state_.get();
            state_reset_timer_.stop();
            state_reset_timer_.start(2000);
            break;
    }
}

void BlobAnimation::ApplyForces(const QVector2D &force) {
    current_blob_state_->ApplyForce(force, velocity_, blob_center_,
                                    control_points_, params_.blob_radius);
}

void BlobAnimation::ApplyIdleEffect() {
    idle_state_->Apply(control_points_, velocity_, blob_center_, params_);
}

void BlobAnimation::setBackgroundColor(const QColor &color) {
    if (params_.background_color != color) {
        params_.background_color = color;
        renderer_.UpdateGridBuffer(params_.background_color, params_.grid_color, params_.grid_spacing, width(),
                                   height());
        update();
    }
}

void BlobAnimation::setGridColor(const QColor &color) {
    if (params_.grid_color != color) {
        params_.grid_color = color;
        renderer_.UpdateGridBuffer(params_.background_color, params_.grid_color, params_.grid_spacing, width(),
                                   height());
        update();
    }
}

void BlobAnimation::setGridSpacing(const int spacing) {
    if (params_.grid_spacing != spacing && spacing > 0) {
        params_.grid_spacing = spacing;
        renderer_.UpdateGridBuffer(params_.background_color, params_.grid_color, params_.grid_spacing, width(),
                                   height());
        update();
    }
}

bool BlobAnimation::eventFilter(QObject *watched, QEvent *event) {
    return event_handler_.eventFilter(watched, event) || QWidget::eventFilter(watched, event);
}

void BlobAnimation::SetLifeColor(const QColor &color) {
    if (default_life_color_.isValid() == false) {
        default_life_color_ = params_.border_color;
    }

    params_.border_color = color;

    needs_redraw_ = true;
    update();
}

void BlobAnimation::ResetLifeColor() {
    if (default_life_color_.isValid()) {
        params_.border_color = default_life_color_;

        needs_redraw_ = true;
        update();
    }
}

void BlobAnimation::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    shader_program_ = new QOpenGLShaderProgram();

    const auto vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";

    const auto fragment_shader_source = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 blobColor;
        uniform vec2 blobCenter;
        uniform float blobRadius;

        void main() {
            // calculate the distance from the center of the blob
            vec2 fragCoord = gl_FragCoord.xy;
            float distance = length(fragCoord - blobCenter);

            // glow effect
            float glow = 1.0 - smoothstep(blobRadius * 0.8, blobRadius, distance);
            FragColor = blobColor * glow;
        }
    )";

    shader_program_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertex_shader_source);
    shader_program_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragment_shader_source);
    shader_program_->link();

    vao_.create();
    vao_.bind();

    vbo_ = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    vbo_.create();
    vbo_.bind();

    shader_program_->enableAttributeArray(0);
    shader_program_->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));

    vbo_.release();
    vao_.release();
}

void BlobAnimation::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    std::lock_guard lock(points_mutex_);

    UpdateBlobGeometry();

    shader_program_->bind();

    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    shader_program_->setUniformValue("projection", projection);

    const QColor blob_color = params_.border_color;

    shader_program_->setUniformValue("blobColor", QVector4D(
                                         blob_color.redF(), blob_color.greenF(), blob_color.blueF(),
                                         blob_color.alphaF()));
    shader_program_->setUniformValue("blobCenter", QVector2D(blob_center_));
    shader_program_->setUniformValue("blobRadius",
                                     static_cast<float>(params_.blob_radius));

    vao_.bind();
    vbo_.bind();
    vbo_.allocate(gl_vertices_.data(), gl_vertices_.size() * sizeof(GLfloat));

    glDrawArrays(GL_TRIANGLE_FAN, 0, gl_vertices_.size() / 2);

    vbo_.release();
    vao_.release();
    shader_program_->release();
}

void BlobAnimation::resizeGL(const int w, const int h) {
    glViewport(0, 0, w, h);
}

void BlobAnimation::UpdateBlobGeometry() {
    gl_vertices_.clear();

    gl_vertices_.push_back(blob_center_.x());
    gl_vertices_.push_back(blob_center_.y());

    for (const auto &point: control_points_) {
        gl_vertices_.push_back(point.x());
        gl_vertices_.push_back(point.y());
    }

    if (!control_points_.empty()) {
        gl_vertices_.push_back(control_points_[0].x());
        gl_vertices_.push_back(control_points_[0].y());
    }
}

void BlobAnimation::DrawGrid() {
    static QOpenGLShaderProgram *grid_shader = nullptr;

    if (!grid_shader) {
        grid_shader = new QOpenGLShaderProgram(this);

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

    grid_shader->bind();

    QMatrix4x4 projection;
    projection.ortho(0, width(), height(), 0, -1, 1);
    grid_shader->setUniformValue("projection", projection);

    const QColor grid_color = params_.grid_color;
    grid_shader->setUniformValue("gridColor", QVector4D(
                                     grid_color.redF(), grid_color.greenF(), grid_color.blueF(),
                                     grid_color.alphaF()));

    QOpenGLBuffer grid_vbo(QOpenGLBuffer::VertexBuffer);
    grid_vbo.create();
    grid_vbo.bind();

    std::vector<GLfloat> grid_vertices;
    const int grid_spacing = params_.grid_spacing;

    // horizontal lines
    for (int y = 0; y < height(); y += grid_spacing) {
        grid_vertices.push_back(0);
        grid_vertices.push_back(y);

        grid_vertices.push_back(width());
        grid_vertices.push_back(y);
    }

    // vertical lines
    for (int x = 0; x < width(); x += grid_spacing) {
        grid_vertices.push_back(x);
        grid_vertices.push_back(0);

        grid_vertices.push_back(x);
        grid_vertices.push_back(height());
    }

    grid_vbo.allocate(grid_vertices.data(), grid_vertices.size() * sizeof(GLfloat));

    // line drawing
    grid_shader->enableAttributeArray(0);
    grid_shader->setAttributeBuffer(0, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));

    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, grid_vertices.size() / 2);

    // free resources
    grid_shader->disableAttributeArray(0);
    grid_vbo.release();
    grid_shader->release();
    grid_vbo.destroy();
}

void BlobAnimation::PauseAllEventTracking() {
    qDebug() << "[BLOB ANIMATION] Pausing all event tracking mechanisms.";
    events_enabled_ = false;
    window_position_timer_.stop();
    state_reset_timer_.stop();
    transition_manager_.ClearAllMovementBuffers();
    event_handler_.DisableEvents();
    SwitchToState(BlobConfig::kIdle);
}

void BlobAnimation::ResumeAllEventTracking() {
    qDebug() << "[BLOB ANIMATION] Resume tracking events.";

    InitializeBlob();
    events_enabled_ = true;
    window_position_timer_.start();
    event_handler_.EnableEvents();
    renderer_.ResetHUD();
    SwitchToState(BlobConfig::kIdle);
    update();
}

void BlobAnimation::setBlobColor(const QColor &color) {
    if (params_.border_color != color) {
        params_.border_color = color;
        update();
    }
}

std::vector<QPointF> BlobAnimation::GenerateOrganicShape(const QPointF &center, const double base_radius,
                                                         const int num_of_points) {
    std::vector<QPointF> points;
    points.reserve(num_of_points);

    std::vector random_factors(num_of_points, 1.0);

    for (int i = 0; i < num_of_points; ++i) {
        constexpr double max_deformation = 1.1;
        constexpr double min_deformation = 0.9;
        const double base_factor = min_deformation + QRandomGenerator::global()->generateDouble() * (
                                       max_deformation - min_deformation);
        random_factors[i] = base_factor;
    }

    const int lobes = 2 + QRandomGenerator::global()->bounded(3); // 2-4 main lobes
    const double lobe_phase = QRandomGenerator::global()->generateDouble() * M_PI * 2; // random phase shifts

    for (int i = 0; i < num_of_points; ++i) {
        const double angle = 2.0 * M_PI * i / num_of_points;
        const double lobe_factor = 0.07 * sin(angle * lobes + lobe_phase);
        const double mid_factor = 0.04 * sin(angle * (lobes * 2 + 1) + lobe_phase * 1.5);

        random_factors[i] = random_factors[i] + lobe_factor + mid_factor;
        random_factors[i] = qBound(0.85, random_factors[i], 1.15);
    }

    std::vector<double> smoothed_factors = random_factors;
    for (int smooth_pass = 0; smooth_pass < 3; ++smooth_pass) {
        std::vector<double> temp_factors = smoothed_factors;
        for (int i = 0; i < num_of_points; ++i) {
            const int prev = i > 0 ? i - 1 : num_of_points - 1;
            const int next = i < num_of_points - 1 ? i + 1 : 0;
            smoothed_factors[i] = temp_factors[prev] * 0.2 +
                                  temp_factors[i] * 0.6 +
                                  temp_factors[next] * 0.2;
        }
    }

    const double max_allowed_radius = base_radius * 0.98; // 2% of the margin for safety

    for (int i = 0; i < num_of_points; ++i) {
        const double angle = 2.0 * M_PI * i / num_of_points;
        // radius scaling for safety boundaries
        double deformed_radius = base_radius * smoothed_factors[i];
        if (deformed_radius > max_allowed_radius) {
            deformed_radius = max_allowed_radius;
        }

        points.emplace_back(
            center.x() + deformed_radius * cos(angle),
            center.y() + deformed_radius * sin(angle)
        );
    }

    return points;
}

void BlobAnimation::ResetBlobToCenter() {
    std::lock_guard lock(points_mutex_);
    const double original_radius = params_.blob_radius;

    control_points_.clear();
    target_points_.clear();
    velocity_.clear();
    blob_center_ = QPointF(width() / 2.0, height() / 2.0);
    control_points_ = GenerateOrganicShape(blob_center_, original_radius, params_.num_of_points);
    target_points_ = control_points_;
    velocity_.resize(params_.num_of_points, QPointF(0, 0));

    if (idle_state_) {
        idle_state_->ResetInitialization();
    }
    SwitchToState(BlobConfig::kIdle);
}

void BlobAnimation::ResetVisualization() {
    ResetBlobToCenter();

    renderer_.ResetHUD();
    renderer_.ForceHUDInitialization(blob_center_, params_.blob_radius,
                                     params_.border_color, width(), height());

    emit visualizationReset();
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
