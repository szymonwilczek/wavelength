#ifndef CONSTANTS_H
#define CONSTANTS_H

// stałe matematyczne
constexpr float M_PI = 3.14159265358979323846f;

// stałe animacji
constexpr int ANIMATION_FPS = 60;
constexpr int ANIMATION_INTERVAL = 1000 / ANIMATION_FPS;

// stałe fizyczne
constexpr float DEFAULT_SPRING_STIFFNESS = 0.001f;
const float DEFAULT_DAMPING_FACTOR = 0.96f;
const float MAX_RADIUS = 0.95f;
const float COLLISION_ENERGY_LOSS = 0.6f;

// oddychanie
const float BREATHE_INCREMENT = 0.015f;

// przesuwanie okna
const float WINDOW_MOVE_FORCE_FACTOR = 0.004f;
const float WINDOW_RESIZE_FORCE_FACTOR = 0.004f;

#endif // CONSTANTS_H