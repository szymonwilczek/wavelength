#include "path_markers_manager.h"

#include <QDateTime>
#include <QPainterPath>
#include <QRandomGenerator>

void PathMarkersManager::InitializeMarkers() {
    markers_.clear();
    last_update_time_ = QDateTime::currentMSecsSinceEpoch();

    QRandomGenerator *rng = QRandomGenerator::global();
    const int num_of_markers = rng->bounded(4, 7);
    int last_marker_type = -1;

    for (int i = 0; i < num_of_markers; i++) {
        PathMarker marker;
        marker.position = rng->bounded(1.0);

        int new_marker_type;
        if (last_marker_type == -1) {
            new_marker_type = rng->bounded(3);
        } else {
            const int offset = 1 + rng->bounded(2);
            new_marker_type = (last_marker_type + offset) % 3;
        }

        marker.marker_type = new_marker_type;
        last_marker_type = new_marker_type;

        marker.size = 4 + rng->bounded(3); // base marker size (4-7)
        marker.direction = rng->bounded(2) * 2 - 1; // marker direction (1-forward or -1-backward)
        marker.color_phase = rng->bounded(1.0);
        marker.color_speed = 0.3 + rng->bounded(0.4);
        marker.tail_length = 0.02 + rng->bounded(0.03);
        marker.wave_phase = 0.0;
        marker.quantum_offset = rng->bounded(10.0);

        switch (marker.marker_type) {
            case 0: // Energy Pulses
                marker.speed = 0.1 + rng->bounded(11) * 0.01;
                break;
            case 1: // Packets
                marker.speed = 0.05;
                break;
            case 2: // Quantum computing
                marker.speed = 0.14 + rng->bounded(4) * 0.02;
                break;
            default:
                break;
        }

        if (marker.marker_type == 2) {
            // Quantum computing
            marker.quantum_state = 0;
            marker.quantum_state_time = 0.0;
            marker.quantum_state_duration = 2.0 + rng->bounded(2.0);
        } else {
            marker.quantum_state = 0;
            marker.quantum_state_time = 0.0;
            marker.quantum_state_duration = 0.0;
        }

        markers_.push_back(marker);
    }
}

void PathMarkersManager::UpdateMarkers(const double delta_time) {
    QRandomGenerator *rng = QRandomGenerator::global();

    for (auto &marker: markers_) {
        marker.position += marker.direction * marker.speed * delta_time;
        if (marker.position > 1.0) {
            marker.position -= 1.0; // looping after reaching the end
        } else if (marker.position < 0.0) {
            marker.position += 1.0; // looping after reaching the start
        }

        marker.color_phase += marker.color_speed * delta_time;
        if (marker.color_phase > 1.0) {
            marker.color_phase -= 1.0;
        }

        if (marker.marker_type == 1) {
            marker.wave_phase += 1.5 * delta_time;
            if (marker.wave_phase > 5.0) {
                marker.wave_phase = 0.0;
            }
        }

        if (marker.marker_type == 2) {
            marker.quantum_state_time += delta_time;

            if (marker.quantum_state_time >= marker.quantum_state_duration) {
                marker.quantum_state = (marker.quantum_state + 1) % 4;
                marker.quantum_state_time = 0.0;

                switch (marker.quantum_state) {
                    case 0: // Single point
                        marker.quantum_state_duration = 2.0 + rng->bounded(2.0);
                        break;
                    case 1: // Expanding
                        marker.quantum_state_duration = 0.8 + rng->bounded(0.6);
                        break;
                    case 2: // Disjointed
                        marker.quantum_state_duration = 1.5 + rng->bounded(1.5);
                        break;
                    case 3: // Retraction
                        marker.quantum_state_duration = 0.8 + rng->bounded(0.6);
                        break;
                    default:
                        break;
                }
            }
        }

        if (marker.marker_type == 0) {
            marker.trail_points.clear();
        }
    }
}

void PathMarkersManager::DrawMarkers(QPainter &painter, const QPainterPath &blob_path, const qint64 current_time) {
    if (markers_.empty()) {
        InitializeMarkers();
    }

    const double delta_time = (current_time - last_update_time_) / 1000.0;
    last_update_time_ = current_time;

    UpdateMarkers(delta_time);

    double path_length = 0;
    for (int i = 0; i < blob_path.elementCount() - 1; i++) {
        QPointF p1 = blob_path.elementAt(i);
        QPointF p2 = blob_path.elementAt(i + 1);
        path_length += QLineF(p1, p2).length();
    }

    for (auto &marker: markers_) {
        const double position = marker.position * path_length;
        double current_length = 0;
        QPointF marker_position;

        for (int j = 0; j < blob_path.elementCount() - 1; j++) {
            QPointF p1 = blob_path.elementAt(j);
            QPointF p2 = blob_path.elementAt(j + 1);
            const double segment_length = QLineF(p1, p2).length();

            if (current_length + segment_length >= position) {
                const double t = (position - current_length) / segment_length;
                marker_position = p1 * (1 - t) + p2 * t;

                if (marker.marker_type == 0) {
                    CalculateTrailPoints(marker, blob_path, position, path_length);
                }

                break;
            }
            current_length += segment_length;
        }

        QColor marker_color = GetMarkerColor(marker.marker_type, marker.color_phase);

        switch (marker.marker_type) {
            case 0:
                DrawImpulseMarker(painter, marker, marker_position, marker_color, current_time);
                break;
            case 1:
                DrawWaveMarker(painter, marker, marker_position, marker_color);
                break;
            case 2:
                DrawQuantumMarker(painter, marker, marker_position, marker_color, current_time);
                break;
            default:
                break;
        }
    }
}

void PathMarkersManager::CalculateTrailPoints(PathMarker &marker, const QPainterPath &blob_path, const double position,
                                              const double path_length) {
    marker.trail_points.clear();
    constexpr int trail_points = 15;

    for (int k = 0; k < trail_points; k++) {
        const double trailT = static_cast<double>(k) / trail_points;

        double trail_pos_on_path = position - marker.direction * trailT * marker.tail_length * path_length;

        // wrapping
        if (trail_pos_on_path < 0) trail_pos_on_path += path_length;
        if (trail_pos_on_path > path_length) trail_pos_on_path -= path_length;

        double trail_length = 0;
        for (int l = 0; l < blob_path.elementCount() - 1; l++) {
            QPointF tp1 = blob_path.elementAt(l);
            QPointF tp2 = blob_path.elementAt(l + 1);
            const double trail_segment_length = QLineF(tp1, tp2).length();

            if (trail_length + trail_segment_length >= trail_pos_on_path) {
                const double trail_point_t = (trail_pos_on_path - trail_length) / trail_segment_length;
                QPointF trail_point = tp1 * (1 - trail_point_t) + tp2 * trail_point_t;
                marker.trail_points.push_back(trail_point);
                break;
            }
            trail_length += trail_segment_length;
        }
    }
}

void PathMarkersManager::DrawImpulseMarker(QPainter &painter, const PathMarker &marker, const QPointF &position,
                                           const QColor &marker_color, qint64 current_time) {
    if (!marker.trail_points.empty()) {
        for (size_t i = 0; i < marker.trail_points.size(); i++) {
            double fade_ratio = static_cast<double>(i) / marker.trail_points.size();
            QColor trail_color = marker_color;
            trail_color.setAlphaF(0.8 - fade_ratio * 0.8);

            int point_size = marker.size * (1.0 - fade_ratio * 0.7);
            painter.setPen(Qt::NoPen);
            painter.setBrush(trail_color);
            painter.drawEllipse(marker.trail_points[i], point_size, point_size);
        }
    }

    const double head_size = marker.size * 1.8;

    // layer 1: Basic shape - rhombus (diamond)
    QPainterPath head;
    head.moveTo(position.x(), position.y() - head_size / 2); // top
    head.lineTo(position.x() + head_size / 2, position.y()); // right
    head.lineTo(position.x(), position.y() + head_size / 2); // down
    head.lineTo(position.x() - head_size / 2, position.y()); // left
    head.closeSubpath();

    QRadialGradient head_gradient(position, head_size / 2);
    QColor core_color = marker_color.lighter(130);
    QColor edge_color = marker_color;
    core_color.setAlphaF(0.9);
    edge_color.setAlphaF(0.7);
    head_gradient.setColorAt(0, core_color);
    head_gradient.setColorAt(1, edge_color);

    painter.setPen(Qt::NoPen);
    painter.setBrush(head_gradient);
    painter.drawPath(head);

    // layer 2: Internal process lines
    QColor tech_color = marker_color.lighter(140);
    tech_color.setAlphaF(0.85);
    painter.setPen(QPen(tech_color, 0.8));

    painter.drawLine(QPointF(position.x() - head_size / 3, position.y()),
                     QPointF(position.x() + head_size / 3, position.y()));
    painter.drawLine(QPointF(position.x(), position.y() - head_size / 3),
                     QPointF(position.x(), position.y() + head_size / 3));

    // layer 3: Central energy point
    QColor center_color = QColor::fromHsvF(
        marker_color.hsvHueF(),
        marker_color.hsvSaturationF() * 0.6,
        qMin(1.0, marker_color.valueF() * 1.4)
    );
    center_color.setAlphaF(0.95);

    painter.setPen(Qt::NoPen);
    painter.setBrush(center_color);
    double inner_size = marker.size * 0.5;

    double time = current_time * 0.002;
    inner_size *= (0.9 + 0.2 * sin(time + marker.position * 10));

    QPainterPath center_hex;
    for (int i = 0; i < 6; i++) {
        double angle = 2.0 * M_PI * i / 6;
        double x = position.x() + inner_size * cos(angle);
        double y = position.y() + inner_size * sin(angle);

        if (i == 0)
            center_hex.moveTo(x, y);
        else
            center_hex.lineTo(x, y);
    }
    center_hex.closeSubpath();
    painter.drawPath(center_hex);

    // layer 4: Flicker effect on the edges (glitch)
    if (QRandomGenerator::global()->bounded(100) < 30) {
        double glitch_angle = QRandomGenerator::global()->bounded(2.0 * M_PI);
        double glitch_dist = head_size / 2 * 0.9;
        QPointF glitch_pos(
            position.x() + cos(glitch_angle) * glitch_dist,
            position.y() + sin(glitch_angle) * glitch_dist
        );

        QColor glitch_color = marker_color.lighter(150);
        glitch_color.setAlphaF(0.7);
        painter.setPen(Qt::NoPen);
        painter.setBrush(glitch_color);
        painter.drawEllipse(glitch_pos, marker.size * 0.3, marker.size * 0.3);
    }
}

void PathMarkersManager::DrawWaveMarker(QPainter &painter, const PathMarker &marker, const QPointF &position,
                                        const QColor &marker_color) {
    if (marker.wave_phase > 0.0) {
        const double wave_radius = marker.size * 1.5 * marker.wave_phase;
        const double opacity = 1.0 - (marker.wave_phase / 5.0);

        QColor wave_color = marker_color;
        wave_color.setAlphaF(opacity * 0.7);

        painter.setPen(QPen(wave_color, 0.6));
        painter.setBrush(Qt::NoBrush);

        for (int i = 0; i < 2; i++) {
            const double ring_radius = wave_radius * (0.7 + 0.3 * i);
            painter.drawEllipse(position, ring_radius, ring_radius);
        }

        QPainterPath distortion_path;
        constexpr int points = 12;
        const double noise_amplitude = wave_radius * 0.07;

        for (int i = 0; i <= points; i++) {
            const double angle = 2.0 * M_PI * i / points;
            const double noise = QRandomGenerator::global()->bounded(noise_amplitude);
            const double x = position.x() + cos(angle) * (wave_radius + noise);
            const double y = position.y() + sin(angle) * (wave_radius + noise);

            if (i == 0)
                distortion_path.moveTo(x, y);
            else
                distortion_path.lineTo(x, y);
        }

        distortion_path.closeSubpath();
        wave_color.setAlphaF(opacity * 0.25);
        painter.setPen(QPen(wave_color, 0.3, Qt::DotLine));
        painter.drawPath(distortion_path);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(marker_color);
    painter.drawEllipse(position, marker.size * 0.8, marker.size * 0.8);
}

void PathMarkersManager::DrawQuantumMarker(QPainter &painter, const PathMarker &marker, const QPointF &position,
                                           const QColor &marker_color, qint64 current_time) {
    double phase = current_time * 0.0005 + marker.quantum_offset;

    double transition_factor = 0.0;
    if (marker.quantum_state == 1) {
        transition_factor = marker.quantum_state_time / marker.quantum_state_duration;
    } else if (marker.quantum_state == 3) {
        transition_factor = 1.0 - marker.quantum_state_time / marker.quantum_state_duration;
    }

    QColor base_color = marker_color;
    if (marker.quantum_state == 0) {
        base_color.setAlphaF(0.9);
    } else {
        base_color.setAlphaF(0.7);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(base_color);
    painter.drawEllipse(position, marker.size * 1.0, marker.size * 1.0);

    QColor inner_color = marker_color.lighter(130);
    inner_color.setAlphaF(base_color.alphaF() * 0.9);
    painter.setBrush(inner_color);
    painter.drawEllipse(position, marker.size * 0.5, marker.size * 0.5);

    if (marker.quantum_state > 0) {
        constexpr int quantum_copies = 5;
        QPainterPath quantum_path;
        QVector<QPointF> quantum_points;

        for (int i = 0; i < quantum_copies; i++) {
            double theta = 2.0 * M_PI * i / quantum_copies;
            double orbit_factor = 0.0;

            if (marker.quantum_state == 2) {
                orbit_factor = 1.0; // full radius
            } else if (marker.quantum_state == 1 || marker.quantum_state == 3) {
                orbit_factor = transition_factor;
            }

            double orbit_a = marker.size * (1.2 + 0.6 * sin(phase * 1.5 + i * 0.7)) * orbit_factor;
            double orbit_b = marker.size * (1.0 + 0.4 * sin(phase * 2.0 + i * 0.5)) * orbit_factor;

            double x = sin(theta + phase) * orbit_a;
            double y = cos(theta + phase * 1.3) * orbit_b;

            QPointF quantum_pos = position + QPointF(x, y);
            quantum_points.append(quantum_pos);

            double pulse_intensity = 0.4 + 0.6 * (0.5 + 0.5 * sin(phase * 3.0 + i));
            double alpha = 0.4 * pulse_intensity * orbit_factor;

            QColor quantum_color = marker_color;
            quantum_color.setAlphaF(alpha);

            painter.setPen(Qt::NoPen);
            painter.setBrush(quantum_color);

            double point_size = marker.size * (0.6 + 0.3 * pulse_intensity) * (0.6 + 0.4 * orbit_factor);
            painter.drawEllipse(quantum_pos, point_size, point_size);

            QColor glow_color = quantum_color.lighter(130);
            glow_color.setAlphaF(quantum_color.alphaF() * 0.8);
            painter.setBrush(glow_color);
            painter.drawEllipse(quantum_pos, point_size * 0.5, point_size * 0.5);

            if (i == 0)
                quantum_path.moveTo(quantum_pos);
            else
                quantum_path.lineTo(quantum_pos);
        }

        if (!quantum_points.isEmpty())
            quantum_path.lineTo(quantum_points.first());

        if (!quantum_points.isEmpty()) {
            QColor path_color = marker_color;
            path_color.setAlphaF(0.2 * (marker.quantum_state == 2 ? 1.0 : transition_factor));
            painter.setPen(QPen(path_color, 0.5, Qt::DotLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(quantum_path);
        }
    }
}

QColor PathMarkersManager::GetMarkerColor(const int marker_type, const double color_phase) {
    switch (marker_type) {
        case 0: // Pulses of energy - Neon green
            return QColor::fromHsvF(0.33 + 0.05 * sin(color_phase * 2 * M_PI), 1.0, 0.95);

        case 1: // Waves of interference - Golden yellow
            return QColor::fromHsvF(0.13 + 0.07 * sin(color_phase * 2 * M_PI), 0.9, 0.95);

        case 2: // Quantum effect - Neo-purpura
            return QColor::fromHsvF(0.78 + 0.1 * sin(color_phase * 2 * M_PI), 0.85, 1.0);

        default:
            return QColor(0, 200, 255); // Default neon blue
    }
}
