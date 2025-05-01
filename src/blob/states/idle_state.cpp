#include "idle_state.h"
#include <cmath>
#include <qmath.h>
#include <QDebug>

IdleState::IdleState() {
    main_phase_offset_ = 0.0;
}

void IdleState::UpdatePhases() {
    // Przyspieszenie fal - zwiększamy inkrementację
    idle_params_.wave_phase += 0.01;  // Było 0.005
    if (idle_params_.wave_phase > 2 * M_PI) {
        idle_params_.wave_phase -= 2 * M_PI;
    }

    second_phase_ += 0.015;  // Było 0.008
    if (second_phase_ > 2 * M_PI) {
        second_phase_ -= 2 * M_PI;
    }

    rotation_phase_ += 0.003;  // Było 0.002
    if (rotation_phase_ > 2 * M_PI) {
        rotation_phase_ -= 2 * M_PI;
    }
}

void IdleState::ResetInitialization() {
    is_initializing_ = true;
    heartbeat_count_ = 0;
    heartbeat_phase_ = 0.0;
}

void IdleState::Apply(std::vector<QPointF>& control_points,
                      std::vector<QPointF>& velocity,
                      QPointF& blob_center,
                      const BlobConfig::BlobParameters& params) {

    // Sprawdzamy, czy jesteśmy w fazie inicjalizacji
    if (is_initializing_) {
        ApplyHeartbeatEffect(control_points, velocity, blob_center, params);
        return;
    }

    // Standardowa animacja idle z falami - istniejący kod
    UpdatePhases();

    const double rotation_strength = 0.15 * std::sin(rotation_phase_ * 0.3);

    const QPointF original_center = blob_center;
    const QPointF screen_center(params.screen_width / 2.0, params.screen_height / 2.0);

    // POPRAWKA: Delikatna siła przyciągająca do środka ekranu
    const QPointF centering_force = (screen_center - blob_center) * 0.01;
    blob_center += centering_force;

    QPointF total_displacement(0, 0);

    for (size_t i = 0; i < control_points.size(); ++i) {
        QPointF vector_from_center = control_points[i] - blob_center;
        const double angle = std::atan2(vector_from_center.y(), vector_from_center.x());
        const double distance_from_center = QVector2D(vector_from_center).length();

        // Fala 1: główna fala obwodowa
        double wave_strength = idle_params_.wave_amplitude * 0.9 *
            std::sin(idle_params_.wave_phase + idle_params_.wave_frequency * angle);

        // Fala 2: dodatkowa fala z inną częstotliwością dla dodania głębi
        wave_strength += idle_params_.wave_amplitude * 0.5 *
            std::sin(second_phase_ + idle_params_.wave_frequency * 2.0 * angle);

        // // Fala 3: fala z zależnością od czasu ale nie od pozycji
        // waveStrength += m_idleParams.waveAmplitude * 0.3 *
        //     std::sin(m_idleParams.wavePhase * 0.7);

        QVector2D normalized_vector = QVector2D(vector_from_center).normalized();
        QPointF perp_vector(-normalized_vector.y(), normalized_vector.x());

        const double rotation_factor = rotation_strength * (distance_from_center / params.blob_radius);
        QPointF rotation_force = rotation_factor * perp_vector;

        QPointF force_vector = QPointF(normalized_vector.x(), normalized_vector.y()) * wave_strength + rotation_force;

        double force_scale = 0.2 + 0.8 * (distance_from_center / params.blob_radius);
        if (force_scale > 1.0) force_scale = 1.0;

        const QPointF delta_force = force_vector * force_scale * 0.15;
        velocity[i] += delta_force;

        total_displacement += delta_force;
    }

    const QPointF avg_displacement = total_displacement / control_points.size();
    for (auto& vel : velocity) {
        vel -= avg_displacement;
    }
    if (QVector2D(blob_center - screen_center).length() > params.blob_radius * 0.1) {
        blob_center = blob_center * 0.95 + screen_center * 0.05;
    } else {
        blob_center = original_center;
    }
}

void IdleState::ApplyForce(const QVector2D& force,
                         std::vector<QPointF>& velocity,
                         QPointF& blob_center,
                         const std::vector<QPointF>& control_points,
                         const double blob_radius) {

    for (size_t i = 0; i < control_points.size(); ++i) {
        QPointF vector_from_center = control_points[i] - blob_center;
        const double distance_from_center = QVector2D(vector_from_center).length();

        double force_scale = distance_from_center / blob_radius;
        if (force_scale > 1.0) force_scale = 1.0;

        velocity[i] += QPointF(
            force.x() * force_scale * 0.8,
            force.y() * force_scale * 0.8
        );
    }

    blob_center += QPointF(force.x() * 0.2, force.y() * 0.2);
}

void IdleState::ApplyHeartbeatEffect(const std::vector<QPointF>& control_points,
                                    std::vector<QPointF>& velocity,
                                    const QPointF& blob_center,
                                    const BlobConfig::BlobParameters& params) {
    // Zmniejsz wartość, aby zwolnić tempo bicia serca
    heartbeat_phase_ += 0.02;  // Zwolnione z 0.03 na 0.015

    if (heartbeat_phase_ >= 2 * M_PI) {
        heartbeat_phase_ -= 2 * M_PI;
        heartbeat_count_++;

        if (heartbeat_count_ >= kRequiredHeartbeats) {
            is_initializing_ = false;
            return;
        }
    }

    // Funkcja pulsu serca z mniejszymi wartościami dla bardziej subtelnego efektu
    double pulse_strength;

    if (heartbeat_phase_ < M_PI * 0.25) {
        // Szybki skurcz - zmniejszony z 1.0 na 0.5
        pulse_strength = 0.5 * std::sin(heartbeat_phase_ * 4);
    } else if (heartbeat_phase_ < M_PI * 0.5) {
        // Krótka pauza - zmniejszona z 0.3 na 0.2
        pulse_strength = 0.3 * std::sin(heartbeat_phase_ * 4);
    } else if (heartbeat_phase_ < M_PI * 0.75) {
        // Powolny rozkurcz - zmniejszony z 0.8 na 0.4
        pulse_strength = 0.4 * std::sin((heartbeat_phase_ - M_PI * 0.5) * 0.8);
    } else {
        // Długa pauza przed następnym biciem - zmniejszona i przyspieszona
        // Zmiana z 0.5 na 0.2 i dodanie mnożnika 1.2 dla kąta, aby skrócić pauzę
        pulse_strength = 0.5 * std::sin(heartbeat_phase_ * 1.2);
    }

    // Stosujemy efekt pulsu do wszystkich punktów kontrolnych
    for (size_t i = 0; i < control_points.size(); ++i) {
        QPointF vector_from_center = control_points[i] - blob_center;
        QVector2D normalized_vector = QVector2D(vector_from_center).normalized();

        // Siła pulsu - ekspansja i kontrakcja
        const double distance_ratio = QVector2D(vector_from_center).length() / params.blob_radius;
        // Zmniejszamy mnożnik z 0.8 na 0.5 dla subtelniejszego efektu
        const double scaled_pulse = pulse_strength * distance_ratio * 0.5;

        const QPointF pulse_force = QPointF(normalized_vector.x(), normalized_vector.y()) * scaled_pulse;
        velocity[i] += pulse_force;
    }
}