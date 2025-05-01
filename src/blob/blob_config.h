#ifndef BLOBCONFIG_H
#define BLOBCONFIG_H

#include <QColor>

namespace BlobConfig {
    enum AnimationState {
        kIdle,
        kMoving,
        kResizing
    };

    struct BlobParameters {
        double blob_radius = 250.0f; // Pozostaw promień, jeśli nie jest konfigurowalny
        int num_of_points = 24;         // Pozostaw liczbę punktów, jeśli nie jest konfigurowalna
        QColor border_color;         // <<< Usunięto domyślny kolor
        int glow_radius = 10;        // Pozostaw, jeśli nie jest konfigurowalne
        int border_width = 6;        // Pozostaw, jeśli nie jest konfigurowalne
        QColor background_color;     // <<< Usunięto domyślny kolor
        QColor grid_color;           // <<< Usunięto domyślny kolor
        int grid_spacing;            // <<< Usunięto domyślny odstęp
        int screen_width = 0;
        int screen_height = 0;
    };

    struct PhysicsParameters {
        double viscosity = 0.015;
        double damping = 0.91;
        double velocity_threshold = 0.1;
        double max_speed = 0.05;
        double restitution = 0.2;
        double stabilization_rate = 0.001;
        double min_neighbor_distance = 0.1;
        double max_neighbor_distance = 0.6;
    };

    struct IdleParameters {
        double wave_amplitude = 2.0;
        double wave_frequency = 3.0;
        double wave_phase = 0.0;
    };
}

#endif // BLOBCONFIG_H