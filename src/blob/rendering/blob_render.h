#ifndef BLOB_RENDERER_H
#define BLOB_RENDERER_H

#include <QPainterPath>
#include <QPixmap>

#include "path_markers_manager.h"
#include "../blob_config.h"

/**
 * @brief Structure holding state information relevant for rendering decisions.
 */
struct BlobRenderState {
    bool is_being_absorbed = false; ///< True if the blob is currently being absorbed by another instance.
    bool is_absorbing = false; ///< True if the blob is currently absorbing another instance.
    bool is_closing_after_absorption = false; ///< True if the application is closing after being absorbed.
    bool is_pulse_active = false; ///< True if a pulse effect should be rendered.
    double opacity = 1.0; ///< Overall opacity multiplier for the blob.
    double scale = 1.0; ///< Overall scale multiplier for the blob.
    BlobConfig::AnimationState animation_state = BlobConfig::kIdle;
    ///< The current animation state (Idle, Moving, Resizing).
};

/**
 * @brief Handles the rendering of the blob, background grid, and HUD elements.
 *
 * This class encapsulates all drawing logic for the Blob animation. It uses QPainter
 * to draw the blob shape (filling, border, glow), the background grid, and informational
 * Heads-Up Display (HUD) elements. It employs caching mechanisms (QPixmap buffers)
 * for the background, grid, glow effect, and HUD to optimize performance, especially
 * when the blob is in the Idle state.
 */
class BlobRenderer {
public:
    /**
     * @brief Constructs a BlobRenderer object.
     * Initializes member variables to default states.
     */
    BlobRenderer() : static_background_initialized_(false), last_grid_spacing_(0), markers_manager_(nullptr),
                     idle_amplitude_(0),
                     idle_hud_initialized_(false),
                     is_rendering_active_(true),
                     last_animation_state_(BlobConfig::kMoving), last_glow_radius_(0) {
        // markers_manager_ = new PathMarkersManager();
    }

    /**
     * @brief Destructor for BlobRenderer.
     * Cleans up allocated resources.
     */
    ~BlobRenderer() {
        delete markers_manager_;
        markers_manager_ = nullptr;
    }

    /**
     * @brief Renders the blob shape itself (filling, border, glow).
     * @param painter The QPainter to use for drawing.
     * @param control_points The current positions of the blob's control points.
     * @param blob_center The calculated center of the blob.
     * @param params Blob appearance parameters (colors, radius, etc.).
     */
    void RenderBlob(QPainter &painter,
                    const std::vector<QPointF> &control_points,
                    const QPointF &blob_center,
                    const BlobConfig::BlobParameters &params);

    /**
     * @brief Updates the internal buffer containing the rendered background grid.
     * Called when grid parameters (colors, spacing) or widget size change.
     * @param background_color The background color to fill behind the grid.
     * @param grid_color The color of the grid lines.
     * @param grid_spacing The spacing between grid lines in pixels.
     * @param width The current width of the rendering area.
     * @param height The current height of the rendering area.
     * @deprecated This method seems redundant as DrawBackground handles buffer updates internally. Consider removing or refactoring.
     */
    void UpdateGridBuffer(const QColor &background_color,
                          const QColor &grid_color,
                          int grid_spacing,
                          int width, int height);

    /**
     * @brief Draws the background, including a static texture and the dynamic grid.
     * Uses internal caching (grid_buffer_) to avoid redrawing the grid every frame if parameters haven't changed.
     * It also draws a static background texture underneath the grid.
     * @param painter The QPainter to use for drawing.
     * @param background_color The desired background color (used for grid buffer update check).
     * @param grid_color The desired color for the grid lines.
     * @param grid_spacing The desired spacing between grid lines.
     * @param width The current width of the rendering area.
     * @param height The current height of the rendering area.
     */
    void DrawBackground(QPainter &painter,
                        const QColor &background_color,
                        const QColor &grid_color,
                        int grid_spacing,
                        int width, int height);

    /**
     * @brief Renders the entire scene, including a background, blob, and potentially HUD.
     * Orchestrates the drawing process, using cached background and HUD elements when in the Idle state.
     * Handles the logic for preparing HUD buffers when transitioning to the Idle state.
     * @param painter The QPainter to use for drawing.
     * @param control_points The current positions of the blob's control points.
     * @param blob_center The calculated center of the blob.
     * @param params Blob appearance parameters.
     * @param render_state Current rendering state information (opacity, scale, animation state, etc.).
     * @param width The current width of the rendering area.
     * @param height The current height of the rendering area.
     * @param background_cache Reference to the QPixmap used for caching the background (managed externally, typically by BlobAnimation).
     * @param last_background_size Reference to the size used for the last background cache update.
     * @param last_background_color Reference to the background color used for the last cache update.
     * @param last_grid_color Reference to the grid color used for the last cache update.
     * @param last_grid_spacing Reference to the grid spacing used for the last cache update.
     */
    void RenderScene(QPainter &painter,
                     const std::vector<QPointF> &control_points,
                     const QPointF &blob_center,
                     const BlobConfig::BlobParameters &params,
                     const BlobRenderState &render_state,
                     int width, int height,
                     QPixmap &background_cache,
                     QSize &last_background_size,
                     QColor &last_background_color,
                     QColor &last_grid_color,
                     int &last_grid_spacing);

    /**
     * @brief Initializes parameters specific to the Idle state HUD display.
     * Generates random ID, calculates amplitude based on time, gets current timestamp.
     * Reset the HUD buffer flags.
     */
    void InitializeIdleState();

    /**
     * @brief Draws the complete static Heads-Up Display (HUD) elements used in the Idle state.
     * Includes corner markers, text labels (ID, radius, amplitude), and a target circle around the blob.
     * @param painter The QPainter to use for drawing.
     * @param blob_center The center position of the blob.
     * @param blob_radius The radius of the blob.
     * @param hud_color The color for the HUD elements.
     * @param width The width of the rendering area.
     * @param height The height of the rendering area.
     */
    void DrawCompleteHUD(QPainter &painter, const QPointF &blob_center, double blob_radius, const QColor &hud_color,
                         int width,
                         int height) const;

    /**
     * @brief Resets the internal grid buffer pixmap, forcing it to be redrawn on the next frame.
     */
    void ResetGridBuffer() {
        grid_buffer_ = QPixmap();
    }

    /**
     * @brief Resets the HUD buffers and flags, forcing reinitialization on the next transition to Idle state.
     */
    void ResetHUD() {
        idle_hud_initialized_ = false;
        static_hud_buffer_ = QPixmap();
    }

    /**
     * @brief Forces the immediate initialization and buffering of the static HUD elements for the Idle state.
     * Calls InitializeIdleState and then renders the HUD into the static_hud_buffer_.
     * @param blob_center The current center of the blob.
     * @param blob_radius The current radius of the blob.
     * @param hud_color The color to use for HUD elements.
     * @param width The current width of the rendering area.
     * @param height The current height of the rendering area.
     */
    void ForceHUDInitialization(const QPointF &blob_center, double blob_radius,
                                const QColor &hud_color, int width, int height);

private:
    /** @brief Cached pixmap containing a static background texture (e.g., gradient, noise). Initialized once. */
    QPixmap static_background_texture_;
    /** @brief Flag indicating if static_background_texture_ has been initialized. */
    bool static_background_initialized_;
    /** @brief Cached pixmap containing the rendered grid lines. Updated when grid parameters or size change. */
    QPixmap grid_buffer_;
    /** @brief Stores the background color used for the last grid_buffer_ update. */
    QColor last_background_color_;
    /** @brief Stores the grid color used for the last grid_buffer_ update. */
    QColor last_grid_color_;
    /** @brief Stores the grid spacing used for the last grid_buffer_ update. */
    int last_grid_spacing_;
    /** @brief Stores the size used for the last grid_buffer_ update. */
    QSize last_size_;
    /** @brief Markers Manager for drawing animated markers alongside blob border (currently unused). */
    PathMarkersManager *markers_manager_;
    /** @brief Randomly generated ID string displayed in the Idle state HUD. */
    QString idle_blob_id_;
    /** @brief Calculated amplitude value displayed in the Idle state HUD. */
    double idle_amplitude_;
    /** @brief Timestamp string displayed in the Idle state HUD (potentially dynamic, currently static after init). */
    QString idle_timestamp_;
    /** @brief Flag indicating if the static HUD elements for the Idle state have been initialized and buffered. */
    bool idle_hud_initialized_;

    /** @brief Flag indicating if rendering is currently active (used for state transition logic). */
    bool is_rendering_active_;
    /** @brief Stores the animation state from the previous frame, used for detecting state changes. */
    BlobConfig::AnimationState last_animation_state_;

    /** @brief Cached pixmap containing the static elements of the HUD rendered during Idle state initialization. */
    QPixmap static_hud_buffer_;

    /** @brief Cached pixmap containing the rendered glow effect. Updated based on blob shape, color, and state. */
    QPixmap glow_buffer_;
    /** @brief Stores the blob path used for the last glow_buffer_ update. */
    QPainterPath last_glow_path_;
    /** @brief Stores the border color used for the last glow_buffer_ update. */
    QColor last_glow_color_;
    /** @brief Stores the glow radius used for the last glow_buffer_ update. */
    int last_glow_radius_;
    /** @brief Stores the viewport size used for the last glow_buffer_ update. */
    QSize last_glow_size_;

    /**
     * @brief Draws the glow effect around the blob path.
     * Uses internal caching (glow_buffer_) to optimize rendering, especially in the Idle state.
     * Delegates the actual drawing to RenderGlowEffect when the buffer needs updating.
     * @param painter The QPainter to use for drawing.
     * @param blob_path The QPainterPath representing the blob's outline.
     * @param border_color The base color for the glow.
     * @param glow_radius The radius/spread of the glow effect.
     */
    void DrawGlowEffect(QPainter &painter,
                        const QPainterPath &blob_path,
                        const QColor &border_color,
                        int glow_radius);

    /**
     * @brief Renders the multi-layered glow effect using QPainter drawPath operations with varying pen widths and colors.
     * This static method performs the actual drawing logic used by DrawGlowEffect when updating the cache.
     * @param painter The QPainter to draw onto (typically the glow buffer painter).
     * @param blob_path The QPainterPath representing the blob's outline.
     * @param border_color The base color for the glow.
     * @param glow_radius The radius/spread of the glow effect.
     */
    static void RenderGlowEffect(QPainter &painter, const QPainterPath &blob_path, const QColor &border_color,
                                 int glow_radius);

    /**
     * @brief Draws the border outline of the blob.
     * Includes a main border and a thinner, lighter inner line for a neon effect.
     * @param painter The QPainter to use for drawing.
     * @param blob_path The QPainterPath representing the blob's outline.
     * @param border_color The color of the main border.
     * @param border_width The width of the main border.
     */
    static void DrawBorder(QPainter &painter,
                           const QPainterPath &blob_path,
                           const QColor &border_color,
                           int border_width);

    /**
     * @brief Draws the filling inside the blob path using a radial gradient.
     * The gradient fades from a lighter, semi-transparent center towards a darker, transparent edge.
     * @param painter The QPainter to use for drawing.
     * @param blob_path The QPainterPath representing the blob's outline.
     * @param blob_center The center point for the radial gradient.
     * @param blob_radius The radius for the radial gradient.
     * @param border_color The base color used to derive gradient colors.
     */
    static void DrawFilling(QPainter &painter,
                            const QPainterPath &blob_path,
                            const QPointF &blob_center,
                            double blob_radius,
                            const QColor &border_color);
};

#endif // BLOB_RENDERER_H
