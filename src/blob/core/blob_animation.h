#ifndef BLOBANIMATION_H
#define BLOBANIMATION_H

#include <vector>
#include <memory>

#include "../blob_config.h"
#include "../physics/blob_physics.h"
#include "../rendering/blob_render.h"
#include "../states/blob_state.h"
#include "../states/idle_state.h"
#include "../states/moving_state.h"
#include "../states/resizing_state.h"
#include "dynamics/blob_event_handler.h"
#include "dynamics/blob_transition_manager.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMutex>

class BlobAnimation final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

    public:
        explicit BlobAnimation(QWidget *parent = nullptr);
        void HandleResizeTimeout();
        void CheckWindowPosition();
        ~BlobAnimation() override;

        QPointF GetBlobCenter() const {
            if (control_points_.empty()) {
               return QPointF(width() / 2.0, height() / 2.0);
            }

            return blob_center_;
        }

        void SetLifeColor(const QColor& color);
        void ResetLifeColor();

        void PauseAllEventTracking();
        void ResumeAllEventTracking();

        void ResetVisualization();

public slots:
    void showAnimation();
    void hideAnimation();
    void setBackgroundColor(const QColor &color);
    void setBlobColor(const QColor &color);
    void setGridColor(const QColor &color);
    void setGridSpacing(int spacing);
    signals:
    void visualizationReset();


protected:
    void paintEvent(QPaintEvent *event) override;

    QRectF CalculateBlobBoundingRect();

    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void UpdateBlobGeometry();

    void DrawGrid();

private slots:
    void updateAnimation();
    void processMovementBuffer();
    void updatePhysics();
    void onStateResetTimeout();

private:
    void InitializeBlob();

    void SwitchToState(BlobConfig::AnimationState new_state);

    void ApplyForces(const QVector2D &force);

    void ApplyIdleEffect();

    void ResetBlobToCenter();

    static std::vector<QPointF> GenerateOrganicShape(const QPointF& center, double base_radius, int num_of_points);

    BlobEventHandler event_handler_;
    BlobTransitionManager transition_manager_;

    QTimer window_position_timer_;
    QPointF last_window_position_timer_;
    QTimer resize_debounce_timer_;
    QSize last_size_;
    QColor default_life_color_;
    bool original_border_color_set_ = false;

    BlobConfig::BlobParameters params_;
    BlobConfig::PhysicsParameters physics_params_;
    BlobConfig::IdleParameters idle_params_;

    std::vector<QPointF> control_points_;
    std::vector<QPointF> target_points_;
    std::vector<QPointF> velocity_;

    QPointF blob_center_;

    BlobConfig::AnimationState current_state_ = BlobConfig::IDLE;

    QTimer animation_timer_;
    QTimer idle_timer_;
    QTimer state_reset_timer_;
    QTimer* transition_to_idle_timer_ = nullptr;

    BlobPhysics physics_;
    BlobRenderer renderer_;

    std::unique_ptr<IdleState> idle_state_;
    std::unique_ptr<MovingState> moving_state_;
    std::unique_ptr<ResizingState> resizing_state_;
    BlobState* current_blob_state_;

    QPointF last_window_position_;

    double precalc_min_distance_ = 0.0;
    double precalc_max_distance_ = 0.0;

    QMutex data_mutex_;
    std::atomic<bool> is_physics_running_{true};
    std::vector<QPointF> safe_control_points_;

    bool events_enabled_ = true;
    QTimer event_re_enable_timer_;
    bool needs_redraw_ = false;

    std::thread physics_thread_;
    std::atomic<bool> physics_active_{true};
    std::mutex points_mutex_;
    std::condition_variable physics_condition_;
    bool needs_update_{false};

    void PhysicsThreadFunction();

    QOpenGLShaderProgram* shader_program_ = nullptr;
    QOpenGLBuffer vbo_;
    QOpenGLVertexArrayObject vao_;

    std::vector<GLfloat> gl_vertices_;
};

#endif // BLOBANIMATION_H