#ifndef BLOB_RENDERER_H
#define BLOB_RENDERER_H

#include <QPainterPath>
#include <vector>

#include "path_markers_manager.h"
#include "../blob_config.h"

struct BlobRenderState {
    bool is_being_absorbed = false;
    bool is_absorbing = false;
    bool is_closing_after_absorption = false;
    bool is_pulse_active = false;
    double opacity = 1.0;
    double scale = 1.0;
    BlobConfig::AnimationState animation_state = BlobConfig::kIdle;
};

class BlobRenderer {
public:

    BlobRenderer() : static_background_initialized_(false), last_grid_spacing_(0),
                     glitch_intensity_(0),
                     last_update_time_(0), idle_amplitude_(0),
                     idle_hud_initialized_(false),
                     is_rendering_active_(true),
                     last_animation_state_(BlobConfig::kMoving), last_glow_radius_(0) {
    }

    void RenderBlob(QPainter &painter,
                    const std::vector<QPointF> &control_points,
                    const QPointF &blob_center,
                    const BlobConfig::BlobParameters &params,
                    BlobConfig::AnimationState animation_state);

    void UpdateGridBuffer(const QColor &background_color,
                          const QColor &grid_color,
                          int grid_spacing,
                          int width, int height);

    void DrawBackground(QPainter &painter,
                        const QColor &background_color,
                        const QColor &grid_color,
                        int grid_spacing,
                        int width, int height);

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

    void InitializeIdleState(const QPointF& blob_center, double blob_radius, const QColor& hud_color, int width,
                             int height);

    void DrawCompleteHUD(QPainter& painter, const QPointF& blob_center, double blob_radius, const QColor& hud_color,
                         int width,
                         int height) const;

    void ResetGridBuffer() {
        grid_buffer_ = QPixmap();
    }

    void SetGlitchIntensity(const double intensity) {
        glitch_intensity_ = intensity;
    }

    void ResetHUD() {
        // Wymuś ponowną inicjalizację HUD
        idle_hud_initialized_ = false;
        static_hud_buffer_ = QPixmap();
    }

    void ForceHUDInitialization(const QPointF& blob_center, double blob_radius,
                           const QColor& hud_color, int width, int height);

private:
    QPixmap static_background_texture_;
    bool static_background_initialized_;
    QPixmap grid_buffer_;
    QColor last_background_color_;
    QColor last_grid_color_;
    int last_grid_spacing_;
    QSize last_size_;
    double glitch_intensity_;
    // PathMarkersManager m_markersManager;
    qint64 last_update_time_;

    QString idle_blob_id_;
    double idle_amplitude_;
    QString idle_timestamp_;
    bool idle_hud_initialized_;
    QPixmap complete_hud_buffer_;

    bool is_rendering_active_;
    BlobConfig::AnimationState last_animation_state_;

    QPixmap static_hud_buffer_;

    QPixmap glow_buffer_;
    QPainterPath last_glow_path_;
    QColor last_glow_color_;
    int last_glow_radius_;
    QSize last_glow_size_;

    void DrawGlowEffect(QPainter &painter,
                       const QPainterPath &blob_path,
                       const QColor &border_color,
                       int glow_radius);

    static void RenderGlowEffect(QPainter& painter, const QPainterPath& blob_path, const QColor& border_color, int glow_radius);

    static void DrawBorder(QPainter &painter,
                           const QPainterPath &blob_path,
                           const QColor &border_color,
                           int border_width);

    static void DrawFilling(QPainter &painter,
                            const QPainterPath &blob_path,
                            const QPointF &blob_center,
                            double blob_radius,
                            const QColor &border_color,
                            BlobConfig::AnimationState animation_state);
};

#endif // BLOB_RENDERER_H