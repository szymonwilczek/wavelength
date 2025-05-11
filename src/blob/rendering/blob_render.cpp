#include "blob_render.h"

#include <QDateTime>
#include <QPainter>
#include <QRandomGenerator>

#include "../utils/blob_path.h"

void BlobRenderer::RenderBlob(QPainter &painter,
                              const std::vector<QPointF> &control_points,
                              const QPointF &blob_center,
                              const BlobConfig::BlobParameters &params) {
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath blob_path = BlobPath::CreateBlobPath(control_points, control_points.size());

    DrawGlowEffect(painter, blob_path, params.border_color, params.glow_radius);
    DrawBorder(painter, blob_path, params.border_color, params.border_width);
    DrawFilling(painter, blob_path, blob_center, params.blob_radius, params.border_color);
}

void BlobRenderer::UpdateGridBuffer(const QColor &background_color,
                                    const QColor &grid_color,
                                    const int grid_spacing,
                                    const int width, const int height) {
    grid_buffer_ = QPixmap(width, height);

    QPainter buffer_painter(&grid_buffer_);
    buffer_painter.setRenderHint(QPainter::Antialiasing, false);

    buffer_painter.fillRect(QRect(0, 0, width, height), background_color);
    buffer_painter.setPen(QPen(grid_color, 1, Qt::SolidLine));

    for (int y = 0; y < height; y += grid_spacing) {
        buffer_painter.drawLine(0, y, width, y);
    }

    for (int x = 0; x < width; x += grid_spacing) {
        buffer_painter.drawLine(x, 0, x, height);
    }

    last_background_color_ = background_color;
    last_grid_color_ = grid_color;
    last_grid_spacing_ = grid_spacing;
    last_size_ = QSize(width, height);
}

void BlobRenderer::DrawBackground(QPainter &painter,
                                  const QColor &background_color,
                                  const QColor &grid_color,
                                  const int grid_spacing,
                                  const int width, const int height) {
    const bool needs_grid_update = grid_buffer_.isNull() ||
                                   background_color != last_background_color_ ||
                                   grid_color != last_grid_color_ ||
                                   grid_spacing != last_grid_spacing_ ||
                                   QSize(width, height) != last_size_;


    if (!static_background_initialized_) {
        constexpr int texture_size = 800;
        static_background_texture_ = QPixmap(texture_size, texture_size);
        static_background_texture_.fill(Qt::transparent);

        QPainter texture_painter(&static_background_texture_);
        texture_painter.setRenderHint(QPainter::Antialiasing, true);

        QLinearGradient background_gradient(0, 0, texture_size, texture_size);
        background_gradient.setColorAt(0, QColor(0, 15, 30));
        background_gradient.setColorAt(1, QColor(10, 25, 40));
        texture_painter.fillRect(QRect(0, 0, texture_size, texture_size), background_gradient);

        static_background_initialized_ = true;
    }

    // static background with points
    painter.drawPixmap(QRect(0, 0, width, height), static_background_texture_);

    // only grid without points (if needed)
    if (needs_grid_update) {
        grid_buffer_ = QPixmap(width, height);
        grid_buffer_.fill(Qt::transparent);

        QPainter buffer_painter(&grid_buffer_);

        const QColor &main_grid_color = grid_color;
        buffer_painter.setPen(QPen(main_grid_color, 1, Qt::SolidLine));

        for (int y = 0; y < height; y += grid_spacing) {
            buffer_painter.drawLine(0, y, width, y);
        }

        for (int x = 0; x < width; x += grid_spacing) {
            buffer_painter.drawLine(x, 0, x, height);
        }

        auto subgrid_color = QColor(grid_color.redF(), grid_color.greenF(), grid_color.blueF(), 0.35);
        subgrid_color.setAlphaF(0.3);
        buffer_painter.setPen(QPen(subgrid_color, 0.5, Qt::DotLine));

        const int sub_grid_spacing = grid_spacing / 2;
        for (int y = sub_grid_spacing; y < height; y += grid_spacing) {
            buffer_painter.drawLine(0, y, width, y);
        }

        for (int x = sub_grid_spacing; x < width; x += grid_spacing) {
            buffer_painter.drawLine(x, 0, x, height);
        }

        last_background_color_ = background_color;
        last_grid_color_ = grid_color;
        last_grid_spacing_ = grid_spacing;
        last_size_ = QSize(width, height);
    }

    painter.drawPixmap(0, 0, grid_buffer_);
}

void BlobRenderer::DrawGlowEffect(QPainter &painter,
                                  const QPainterPath &blob_path,
                                  const QColor &border_color,
                                  const int glow_radius) {
    const auto viewport_size = QSize(painter.device()->width(), painter.device()->height());

    const bool buffer_needs_update = glow_buffer_.isNull() ||
                                     blob_path != last_glow_path_ ||
                                     border_color != last_glow_color_ ||
                                     glow_radius != last_glow_radius_ ||
                                     viewport_size != last_glow_size_;

    if (last_animation_state_ == BlobConfig::kIdle && !buffer_needs_update && !glow_buffer_.isNull()) {
        painter.drawPixmap(0, 0, glow_buffer_);
        return;
    }

    static int frame_counter = 0;
    const bool should_update_buffer = buffer_needs_update ||
                                      (last_animation_state_ != BlobConfig::kIdle &&
                                       frame_counter++ % 5 == 0);

    if (should_update_buffer) {
        QPixmap new_glow_buffer(viewport_size);
        new_glow_buffer.fill(Qt::transparent);

        QPainter buffer_painter(&new_glow_buffer);
        buffer_painter.setRenderHint(QPainter::Antialiasing, true);

        RenderGlowEffect(buffer_painter, blob_path, border_color, glow_radius);
        buffer_painter.end();

        glow_buffer_ = new_glow_buffer;

        last_glow_path_ = blob_path;
        last_glow_color_ = border_color;
        last_glow_radius_ = glow_radius;
        last_glow_size_ = viewport_size;
    }

    painter.drawPixmap(0, 0, glow_buffer_);
}

void BlobRenderer::RenderGlowEffect(QPainter &painter,
                                    const QPainterPath &blob_path,
                                    const QColor &border_color,
                                    int glow_radius) {
    painter.save();

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // 1. base layer (mild glow) - the outermost layer
    {
        QColor outer_color = border_color;
        outer_color = QColor::fromHslF(
            outer_color.hslHueF(),
            qMin(0.9, outer_color.hslSaturationF() * 0.7),
            outer_color.lightnessF(),
            0.2
        );

        QPen outer_pen(outer_color);
        outer_pen.setWidth(glow_radius);
        outer_pen.setCapStyle(Qt::RoundCap);
        outer_pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(outer_pen);
        painter.drawPath(blob_path);
    }

    // 2. middle layer (intense glow) - typical of neon signs
    {
        QColor mid_color = border_color.lighter(115);
        mid_color = QColor::fromHslF(
            mid_color.hslHueF(),
            qMin(1.0, mid_color.hslSaturationF() * 1.1),
            qMin(0.9, mid_color.lightnessF() * 1.2),
            0.6
        );

        QPen mid_pen(mid_color);
        mid_pen.setWidth(glow_radius / 2);
        mid_pen.setCapStyle(Qt::RoundCap);
        mid_pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(mid_pen);
        painter.drawPath(blob_path);
    }

    // 3. inner layer (bright nucleus) - characteristic of neon-ish effects
    {
        QColor core_color = border_color.lighter(160);
        core_color = QColor::fromHslF(
            core_color.hslHueF(),
            qMin(0.3, core_color.hslSaturationF() * 0.5),
            0.9,
            0.95
        );

        QPen core_pen(core_color);
        core_pen.setWidth(3);
        core_pen.setCapStyle(Qt::RoundCap);
        core_pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(core_pen);
        painter.drawPath(blob_path);
    }

    painter.restore();
}

void BlobRenderer::DrawBorder(QPainter &painter,
                              const QPainterPath &blob_path,
                              const QColor &border_color,
                              const int border_width) {
    QPen border_pen(border_color);
    border_pen.setWidth(border_width);
    painter.setPen(border_pen);
    painter.drawPath(blob_path);

    QPen inner_pen(border_color.lighter(150));
    inner_pen.setWidth(1);
    painter.setPen(inner_pen);
    painter.drawPath(blob_path);

    // if (markers_manager_) {
    //     const qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    //     markers_manager_->drawMarkers(painter, blob_path, currentTime);
    // }
}


void BlobRenderer::DrawFilling(QPainter &painter,
                               const QPainterPath &blob_path,
                               const QPointF &blob_center,
                               double blob_radius,
                               const QColor &border_color) {
    QRadialGradient gradient(blob_center, blob_radius);

    // brighter middle
    QColor center_color = border_color.lighter(130);
    center_color.setAlpha(30);
    gradient.setColorAt(0, center_color);

    QColor mid_color = border_color;
    mid_color.setAlpha(15);
    gradient.setColorAt(0.7, mid_color);

    QColor edge_color = border_color.darker(120);
    edge_color.setAlpha(5);
    gradient.setColorAt(0.9, edge_color);
    gradient.setColorAt(1, QColor(0, 0, 0, 0));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(blob_path);
}

void BlobRenderer::RenderScene(QPainter &painter,
                               const std::vector<QPointF> &control_points,
                               const QPointF &blob_center,
                               const BlobConfig::BlobParameters &params,
                               const BlobRenderState &render_state,
                               const int width, const int height,
                               QPixmap &background_cache,
                               QSize &last_background_size,
                               QColor &last_background_color,
                               QColor &last_grid_color,
                               int &last_grid_spacing) {
    const bool state_changing_to_idle = render_state.animation_state == BlobConfig::kIdle && last_animation_state_ !=
                                        BlobConfig::kIdle;

    if (state_changing_to_idle) {
        InitializeIdleState();

        static_hud_buffer_ = QPixmap(width, height);
        static_hud_buffer_.fill(Qt::transparent);

        QPainter hud_painter(&static_hud_buffer_);
        hud_painter.setRenderHint(QPainter::Antialiasing, true);
        DrawCompleteHUD(hud_painter, blob_center, params.blob_radius, params.border_color, width, height);
        hud_painter.end();

        idle_hud_initialized_ = true;
    }

    if (state_changing_to_idle) {
        is_rendering_active_ = false;
    } else if (render_state.animation_state != BlobConfig::kIdle) {
        is_rendering_active_ = true;
    }

    last_animation_state_ = render_state.animation_state;

    const bool should_update_background_cache =
            background_cache.isNull() ||
            last_background_size != QSize(width, height) ||
            last_background_color != params.background_color ||
            last_grid_color != params.grid_color ||
            last_grid_spacing != params.grid_spacing;

    if (render_state.animation_state == BlobConfig::kIdle) {
        if (should_update_background_cache) {
            background_cache = QPixmap(width, height);
            background_cache.fill(Qt::transparent);

            QPainter cache_painter(&background_cache);
            cache_painter.setRenderHint(QPainter::Antialiasing, true);
            DrawBackground(cache_painter, params.background_color,
                           params.grid_color, params.grid_spacing,
                           width, height);

            last_background_size = QSize(width, height);
            last_background_color = params.background_color;
            last_grid_color = params.grid_color;
            last_grid_spacing = params.grid_spacing;
        }

        painter.drawPixmap(0, 0, background_cache);

        RenderBlob(painter, control_points, blob_center, params);

        if (idle_hud_initialized_ && !static_hud_buffer_.isNull()) {
            painter.drawPixmap(0, 0, static_hud_buffer_);
        }
    } else {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        DrawBackground(painter, params.background_color,
                       params.grid_color, params.grid_spacing,
                       width, height);

        RenderBlob(painter, control_points, blob_center, params);
    }
}

void BlobRenderer::InitializeIdleState() {
    idle_blob_id_ = QString("BLOB-ID: %1").arg(QRandomGenerator::global()->bounded(1000, 9999));
    idle_amplitude_ = 1.5 + sin(QDateTime::currentMSecsSinceEpoch() * 0.001) * 0.5;
    idle_timestamp_ = QDateTime::currentDateTime().toString("HH:mm:ss");

    static_hud_buffer_ = QPixmap();
    idle_hud_initialized_ = false;
}


void BlobRenderer::DrawCompleteHUD(QPainter &painter, const QPointF &blob_center,
                                   const double blob_radius, const QColor &hud_color,
                                   const int width, const int height) const {
    QColor tech_color = hud_color.lighter(120);
    tech_color.setAlpha(180);
    painter.setPen(QPen(tech_color, 1));
    painter.setFont(QFont("Consolas", 8));

    constexpr int corner_size = 15;

    // left-top
    painter.drawLine(10, 10, 10 + corner_size, 10);
    painter.drawLine(10, 10, 10, 10 + corner_size);
    painter.drawText(15, 25, "BLOB MONITOR");

    // right-top
    painter.drawLine(width - 10 - corner_size, 10, width - 10, 10);
    painter.drawLine(width - 10, 10, width - 10, 10 + corner_size);

    // Prawy dolny
    painter.drawLine(width - 10 - corner_size, height - 10, width - 10, height - 10);
    painter.drawLine(width - 10, height - 10, width - 10, height - 10 - corner_size);
    painter.drawText(width - 70, height - 25, QString("R: %1").arg(static_cast<int>(blob_radius)));

    // Lewy dolny
    painter.drawLine(10, height - 10, 10 + corner_size, height - 10);
    painter.drawLine(10, height - 10, 10, height - 10 - corner_size);
    painter.drawText(15, height - 25, QString("AMP: %1").arg(idle_amplitude_, 0, 'f', 1)); // Ustalona amplituda

    // circle around the blob (target)
    const QPen target_pen(tech_color, 1, Qt::DotLine);
    painter.setPen(target_pen);
    painter.drawEllipse(blob_center, blob_radius + 20, blob_radius + 20);

    // blob-id
    const QFontMetrics font_metrics(painter.font());
    const int text_width = font_metrics.horizontalAdvance(idle_blob_id_);
    painter.drawText(blob_center.x() - text_width / 2, blob_center.y() + blob_radius + 30, idle_blob_id_);
}

void BlobRenderer::ForceHUDInitialization(const QPointF &blob_center, const double blob_radius, const QColor &hud_color,
                                          const int width, const int height) {
    InitializeIdleState();

    static_hud_buffer_ = QPixmap(width, height);
    static_hud_buffer_.fill(Qt::transparent);

    QPainter hud_painter(&static_hud_buffer_);
    hud_painter.setRenderHint(QPainter::Antialiasing, true);
    DrawCompleteHUD(hud_painter, blob_center, blob_radius, hud_color, width, height);
    hud_painter.end();

    idle_hud_initialized_ = true;
}
