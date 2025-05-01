#ifndef BLOBANIMATION_H
#define BLOBANIMATION_H

#include <vector>
#include <memory>
#include <thread> // Added for std::thread
#include <mutex> // Added for std::mutex
#include <condition_variable> // Added for std::condition_variable
#include <atomic> // Added for std::atomic

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
#include <QTimer>
#include <QPaintEvent>

/**
 * @brief Main widget responsible for rendering and animating the dynamic blob.
 *
 * This class integrates physics simulation (BlobPhysics), rendering (BlobRenderer),
 * state management (IdleState, MovingState, ResizingState), event handling (BlobEventHandler),
 * and transition logic (BlobTransitionManager) to create a fluid, interactive blob animation.
 * It uses QOpenGLWidget for hardware-accelerated rendering and runs physics calculations
 * in a separate thread for performance.
 */
class BlobAnimation final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    /**
     * @brief Constructs the BlobAnimation widget.
     * Initializes parameters, states, event handlers, timers, and the physics thread.
     * Sets up OpenGL format and connects signals/slots between components.
     * @param parent Optional parent widget.
     */
    explicit BlobAnimation(QWidget *parent = nullptr);

    /**
     * @brief Destructor.
     * Stops timers, cleans up OpenGL resources, joins the physics thread, and deletes state objects.
     */
    ~BlobAnimation() override;

    /**
     * @brief Gets the current calculated center position of the blob.
     * If control points are not yet initialized, returns the center of the widget.
     * @return The blob's center position as a QPointF.
     */
    QPointF GetBlobCenter() const {
        std::lock_guard lock(points_mutex_); // Ensure thread safety when accessing blob_center_
        if (control_points_.empty()) {
            return QPointF(width() / 2.0, height() / 2.0);
        }
        return blob_center_;
    }

    /**
     * @brief Temporarily sets the blob's border color (e.g., for highlighting).
     * Stores the original color to be restored later by ResetLifeColor().
     * @param color The temporary color to apply.
     */
    void SetLifeColor(const QColor& color);

    /**
     * @brief Resets the blob's border color to its original value before SetLifeColor() was called.
     */
    void ResetLifeColor();

    /**
     * @brief Pauses all event tracking mechanisms (window move/resize).
     * Stops timers, clears movement buffers, and disables the event handler.
     * Switches the blob state to Idle.
     */
    void PauseAllEventTracking();

    /**
     * @brief Resumes all event tracking mechanisms after being paused.
     * Restarts timers, enables the event handler, resets the blob state, and forces a redraw.
     */
    void ResumeAllEventTracking();

    /**
     * @brief Resets the entire blob visualization to its initial state.
     * Resets blob position, shape, and dynamics, resets the HUD, and emits visualizationReset().
     */
    void ResetVisualization();

public slots:
    /**
     * @brief Makes the animation widget visible and resumes event tracking.
     */
    void showAnimation();

    /**
     * @brief Hides the animation widget and pauses event tracking.
     */
    void hideAnimation();

    /**
     * @brief Sets the background color used for rendering.
     * Updates the grid buffer and triggers a repaint.
     * @param color The new background color.
     */
    void setBackgroundColor(const QColor &color);

    /**
     * @brief Sets the border color of the blob.
     * Triggers a repaint.
     * @param color The new blob border color.
     */
    void setBlobColor(const QColor &color);

    /**
     * @brief Sets the color of the background grid lines.
     * Updates the grid buffer and triggers a repaint.
     * @param color The new grid color.
     */
    void setGridColor(const QColor &color);

    /**
     * @brief Sets the spacing between background grid lines.
     * Updates the grid buffer and triggers a repaint.
     * @param spacing The new grid spacing in pixels.
     */
    void setGridSpacing(int spacing);

signals:
    /**
     * @brief Emitted when the entire visualization is reset via ResetVisualization().
     * Allows other UI elements to update accordingly.
     */
    void visualizationReset();


protected:
    /**
     * @brief Overridden paint event handler.
     * Delegates rendering to BlobRenderer after checking if the paint area intersects the blob's bounding box.
     * Uses a cached background pixmap for efficiency.
     * @param event The paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Calculates the bounding rectangle encompassing the blob's control points, including margins for border and glow.
     * Used for optimizing paint events.
     * @return The calculated bounding rectangle.
     */
    QRectF CalculateBlobBoundingRect();

    /**
     * @brief Overridden resize event handler.
     * Delegates processing to BlobEventHandler.
     * @param event The resize event.
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief Overridden generic event handler.
     * @param event The event.
     * @return Result of QWidget::event(event).
     */
    bool event(QEvent *event) override;

    /**
     * @brief Overridden event filter.
     * Delegates filtering primarily to BlobEventHandler.
     * @param watched The object being watched.
     * @param event The event.
     * @return True if the event was handled, false otherwise.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    // --- OpenGL Methods ---
    /**
     * @brief Initializes OpenGL functions, shaders, VAO, and VBO. Called once before the first paintGL call.
     */
    void initializeGL() override;

    /**
     * @brief Renders the blob and grid using OpenGL. Called whenever the widget needs to be painted.
     * Binds shaders, sets uniforms (projection matrix, colors, etc.), updates VBO data, and draws the blob geometry.
     */
    void paintGL() override;

    /**
     * @brief Handles OpenGL viewport updates when the widget is resized.
     * @param w New width.
     * @param h New height.
     */
    void resizeGL(int w, int h) override;

    /**
     * @brief Updates the internal vertex buffer (gl_vertices_) based on the current blob_center_ and control_points_.
     * Prepares the geometry data for rendering with GL_TRIANGLE_FAN.
     */
    void UpdateBlobGeometry();

    /**
     * @brief Draws the background grid using a separate shader program and line primitives.
     * Generates vertex data for horizontal and vertical lines based on grid spacing.
     */
    void DrawGrid();
    // --- End OpenGL Methods ---

private slots:
    /**
     * @brief Slot connected to animation_timer_. Called periodically (~60 FPS) to update the animation state.
     * Processes movement buffer, applies current state logic, and triggers a repaint if needed.
     */
    void updateAnimation();

    /**
     * @brief Slot (or helper called by updateAnimation) to process the window movement buffer via BlobTransitionManager.
     * Provides callbacks to apply inertia forces and update the last known window position.
     */
    void processMovementBuffer();

    /**
     * @brief Slot (or helper called by physics thread) to update the blob's physics simulation.
     * Calls BlobPhysics methods for internal forces, collisions, smoothing, and constraints.
     */
    void updatePhysics();

    /**
     * @brief Slot connected to state_reset_timer_. Called after a period of inactivity in Moving or Resizing state.
     * Automatically switches the state back to Idle.
     */
    void onStateResetTimeout();

    /**
     * @brief Slot connected to resize_debounce_timer_. Called after a short delay following resize events.
     * Resets the blob to the center, resets the grid buffer, and updates the widget.
     */
    void HandleResizeTimeout();

    /**
     * @brief Slot connected to window_position_timer_. Called frequently to check the window position.
     * Adds position samples to the BlobTransitionManager if the window has moved or if the buffer is stale.
     */
    void CheckWindowPosition();


private:
    /**
     * @brief Initializes the blob's state (control points, target points, velocity, center)
     * based on current parameters. Generates the initial organic shape.
     */
    void InitializeBlob();

    /**
     * @brief Switches the blob's current animation state (Idle, Moving, Resizing).
     * Updates the current_blob_state_ pointer and handles state-specific logic like timer management
     * and resetting state properties.
     * @param new_state The target state to switch to.
     */
    void SwitchToState(BlobConfig::AnimationState new_state);

    /**
     * @brief Applies an external force vector to the blob.
     * Delegates the force application to the current state object.
     * @param force The force vector to apply.
     */
    void ApplyForces(const QVector2D &force);

    /**
     * @brief Explicitly applies the idle state effect (e.g., heartbeat).
     * Calls the Apply method of the idle_state_ object.
     */
    void ApplyIdleEffect();

    /**
     * @brief Resets the blob's position to the center of the widget and regenerates its shape.
     * Clears velocities and switches the state to Idle.
     */
    void ResetBlobToCenter();

    /**
     * @brief Generates a set of points forming an organic, blob-like shape around a center point.
     * Uses random factors combined with smoothing and sinusoidal variations to create irregularity.
     * @param center The center point for the shape.
     * @param base_radius The average radius of the shape.
     * @param num_of_points The number of control points to generate.
     * @return A vector of QPointF representing the generated control points.
     */
    static std::vector<QPointF> GenerateOrganicShape(const QPointF& center, double base_radius, int num_of_points);

    /**
     * @brief The main function executed by the physics simulation thread.
     * Enters a loop that repeatedly calls updatePhysics(), notifies the UI thread to update,
     * and waits for the next frame time, controlled by physics_active_ flag and physics_condition_.
     */
    void PhysicsThreadFunction();

    // --- Member Variables ---

    /** @brief Handles window move/resize event filtering and processing. */
    BlobEventHandler event_handler_;
    /** @brief Manages movement analysis, velocity calculation, and state transitions based on movement. */
    BlobTransitionManager transition_manager_;

    /** @brief Timer for periodically checking the window position. */
    QTimer window_position_timer_;
    /** @brief Stores the last window position recorded by the window_position_timer_. */
    QPointF last_window_position_timer_;
    /** @brief Timer for debouncing resize events. */
    QTimer resize_debounce_timer_;
    /** @brief Stores the last known size of the widget after a resize event. */
    QSize last_size_;
    /** @brief Stores the default border color before SetLifeColor is called. */
    QColor default_life_color_;
    /** @brief Flag indicating if default_life_color_ has been set. */
    bool original_border_color_set_ = false;

    /** @brief Structure holding general blob appearance parameters (colors, radius, points, etc.). */
    BlobConfig::BlobParameters params_;
    /** @brief Structure holding physics simulation parameters (stiffness, damping, etc.). */
    BlobConfig::PhysicsParameters physics_params_;
    /** @brief Structure holding parameters specific to the idle state animation (wave amplitude/frequency). */
    BlobConfig::IdleParameters idle_params_;

    /** @brief Vector storing the current positions of the blob's control points. Modified by physics and states. Protected by points_mutex_. */
    std::vector<QPointF> control_points_;
    /** @brief Vector storing the target positions for control points (used by physics). Protected by points_mutex_. */
    std::vector<QPointF> target_points_;
    /** @brief Vector storing the current velocities of the blob's control points. Protected by points_mutex_. */
    std::vector<QPointF> velocity_;

    /** @brief The calculated center position of the blob. Protected by points_mutex_. */
    QPointF blob_center_;

    /** @brief The current animation state (Idle, Moving, Resizing). */
    BlobConfig::AnimationState current_state_ = BlobConfig::kIdle;

    /** @brief Main timer driving the animation updates (~60 FPS). */
    QTimer animation_timer_;
    /** @brief Timer potentially used for idle state effects (may be deprecated or integrated elsewhere). */
    QTimer idle_timer_;
    /** @brief Timer used to automatically switch back to Idle state after inactivity in Moving/Resizing state. */
    QTimer state_reset_timer_;
    /** @brief Timer potentially used for idle transition animation (may be deprecated). */
    QTimer* transition_to_idle_timer_ = nullptr;

    /** @brief Object responsible for calculating physics interactions. */
    BlobPhysics physics_;
    /** @brief Object responsible for rendering the blob, grid, and HUD. */
    BlobRenderer renderer_;

    /** @brief Unique pointer to the Idle state logic object. */
    std::unique_ptr<IdleState> idle_state_;
    /** @brief Unique pointer to the Moving state logic object. */
    std::unique_ptr<MovingState> moving_state_;
    /** @brief Unique pointer to the Resizing state logic object. */
    std::unique_ptr<ResizingState> resizing_state_;
    /** @brief Pointer to the currently active state object (points to one of the above). */
    BlobState* current_blob_state_;

    /** @brief Stores the last known window position (potentially used by physics). */
    QPointF last_window_position_;

    /** @brief Precalculated minimum distance constraint for physics. */
    double precalc_min_distance_ = 0.0;
    /** @brief Precalculated maximum distance constraint for physics. */
    double precalc_max_distance_ = 0.0;

    /** @brief Mutex protecting access to shared data between UI and physics threads (control_points_, velocity_, etc.). */
    mutable std::mutex points_mutex_; // Marked mutable to allow locking in const methods like GetBlobCenter
    /** @brief Atomic flag controlling the physics thread loop. */
    std::atomic<bool> physics_active_{true};
    /** @brief Condition variable used by the physics thread to wait for the next frame time. */
    std::condition_variable physics_condition_;

    /** @brief Flag indicating if event processing is currently enabled. */
    bool events_enabled_ = true;
    /** @brief Timer for delayed re-enabling of events after transitions. */
    QTimer event_re_enable_timer_;
    /** @brief Flag indicating if a redraw is needed in the next animation frame. */
    bool needs_redraw_ = false;

    /** @brief The separate thread object running the physics simulation (PhysicsThreadFunction). */
    std::thread physics_thread_;

    // --- OpenGL Specific Members ---
    /** @brief Pointer to the shader program used for rendering the blob. */
    QOpenGLShaderProgram* shader_program_ = nullptr;
    /** @brief Vertex Buffer Object storing blob geometry data. */
    QOpenGLBuffer vbo_;
    /** @brief Vertex Array Object encapsulating vertex buffer state. */
    QOpenGLVertexArrayObject vao_;
    /** @brief Vector storing vertex data (x, y pairs) to be uploaded to the VBO. */
    std::vector<GLfloat> gl_vertices_;
    // --- End OpenGL Specific Members ---
};

#endif // BLOBANIMATION_H