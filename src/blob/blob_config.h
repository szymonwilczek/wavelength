#ifndef BLOBCONFIG_H
#define BLOBCONFIG_H

#include <QColor>

/**
 * @brief Namespace containing configuration structures and enums for the Blob animation.
 *
 * This namespace groups together parameters related to the blob's appearance,
 * physics simulation, idle state behavior, and overall animation state.
 */
namespace BlobConfig {
    /**
     * @brief Defines the possible animation states for the blob.
     */
    enum AnimationState {
        kIdle, ///< The blob is stationary or exhibiting subtle idle animations.
        kMoving, ///< The blob is reacting to window movement (inertia).
        kResizing ///< The blob is reacting to window resizing.
    };

    /**
     * @brief Structure holding parameters related to the blob's visual appearance and basic geometry.
     */
    struct BlobParameters {
        /** @brief The base radius of the blob shape in pixels. */
        double blob_radius = 250.0f;
        /** @brief The number of control points used to define the blob's shape. */
        int num_of_points = 24;
        /** @brief The color of the blob's border and fill gradient base. */
        QColor border_color;
        /** @brief The radius/spread of the glow effect around the border in pixels. */
        int glow_radius = 10;
        /** @brief The width of the main borderline in pixels. */
        int border_width = 6;
        /** @brief The background color of the widget area. */
        QColor background_color;
        /** @brief The color of the grid lines drawn on the background. */
        QColor grid_color;
        /** @brief The spacing between grid lines in pixels. */
        int grid_spacing;
        /** @brief The current width of the screen/widget area (used for border collisions, etc.). */
        int screen_width = 0;
        /** @brief The current height of the screen/widget area (used for border collisions, etc.). */
        int screen_height = 0;
    };

    /**
     * @brief Structure holding parameters controlling the physics simulation of the blob.
     */
    struct PhysicsParameters {
        /** @brief Resistance to internal deformation (higher values make it more rigid). */
        double viscosity = 0.015;
        /** @brief Factor by which velocity is reduced each frame (0.0 to 1.0). Controls how quickly movement stops. */
        double damping = 0.91;
        /** @brief Minimum velocity magnitude considered significant (used for state transitions). */
        double velocity_threshold = 0.1;
        /** @brief Maximum speed limit for individual control points (prevents instability). */
        double max_speed = 0.05;
        /** @brief Bounciness factor for border collisions (0.0 = no bounce, 1.0 = full bounce). */
        double restitution = 0.2;
        /** @brief Rate at which the blob returns to a more circular shape when idle (0.0 to 1.0). */
        double stabilization_rate = 0.001;
        /** @brief Minimum allowed distance between neighboring control points (relative to radius). */
        double min_neighbor_distance = 0.1;
        /** @brief Maximum allowed distance between neighboring control points (relative to radius). */
        double max_neighbor_distance = 0.6;
    };

    /**
     * @brief Structure holding parameters specific to the Idle state animation.
     */
    struct IdleParameters {
        /** @brief Amplitude (magnitude) of the wave effect applied in the Idle state. */
        double wave_amplitude = 2.0;
        /** @brief Frequency (speed) of the wave effect applied in the Idle state. */
        double wave_frequency = 3.0;
        /** @brief Current phase offset of the wave effect (updated over time). */
        double wave_phase = 0.0;
    };
}

#endif // BLOBCONFIG_H
