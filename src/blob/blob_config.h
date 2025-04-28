#ifndef BLOBCONFIG_H
#define BLOBCONFIG_H

#include <QColor>

namespace BlobConfig {
    enum AnimationState {
        IDLE,
        MOVING,
        RESIZING
    };

    struct BlobParameters {
        double blobRadius = 250.0f; // Pozostaw promień, jeśli nie jest konfigurowalny
        int numPoints = 24;         // Pozostaw liczbę punktów, jeśli nie jest konfigurowalna
        QColor borderColor;         // <<< Usunięto domyślny kolor
        int glowRadius = 10;        // Pozostaw, jeśli nie jest konfigurowalne
        int borderWidth = 6;        // Pozostaw, jeśli nie jest konfigurowalne
        QColor backgroundColor;     // <<< Usunięto domyślny kolor
        QColor gridColor;           // <<< Usunięto domyślny kolor
        int gridSpacing;            // <<< Usunięto domyślny odstęp
        int screenWidth = 0;
        int screenHeight = 0;
    };

    struct PhysicsParameters {
        double viscosity = 0.015;
        double damping = 0.91;
        double velocityThreshold = 0.1;
        double maxSpeed = 0.05;
        double restitution = 0.2;
        double stabilizationRate = 0.001;
        double minNeighborDistance = 0.1;
        double maxNeighborDistance = 0.6;
    };

    struct IdleParameters {
        double waveAmplitude = 2.0;
        double waveFrequency = 3.0;
        double wavePhase = 0.0;
    };
}

#endif // BLOBCONFIG_H