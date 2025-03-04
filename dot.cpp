#include "dot.h"
#include "constants.h"
#include <cmath>

Dot::Dot() : baseX(0), baseY(0), x(0), y(0), velX(0), velY(0),
             size(10), hue(0), mass(1.0f) {
}

Dot::Dot(const float angle, float baseRadius, int index, int totalDots) {
    baseX = cos(angle) * baseRadius;
    baseY = sin(angle) * baseRadius;
    x = baseX;
    y = baseY;
    velX = 0;
    velY = 0;
    size = 2 + (index % 5);
    hue = index * 360.0f / totalDots;
    mass = size / 10.0f;
}

void Dot::update(float springStiffness, float dampingFactor) {
    float forceX = (baseX - x) * springStiffness;
    float forceY = (baseY - y) * springStiffness;

    // prędkość
    velX += forceX;
    velY += forceY;

    // tłumienie
    velX *= dampingFactor;
    velY *= dampingFactor;

    // pozycja
    x += velX;
    y += velY;

    checkBoundaries();
}

void Dot::applyForce(float forceX, float forceY) {
    velX += forceX / mass;
    velY += forceY / mass;
}

void Dot::checkBoundaries() {
    float distanceFromCenter = sqrt(x * x + y * y);

    if (distanceFromCenter > MAX_RADIUS) {
        float normFactor = MAX_RADIUS / distanceFromCenter;
        x *= normFactor;
        y *= normFactor;

        // normalizacja składowych
        float nx = x / distanceFromCenter;
        float ny = y / distanceFromCenter;

        float dotProduct = velX * nx + velY * ny;

        if (dotProduct > 0) {
            velX -= 2 * dotProduct * nx * COLLISION_ENERGY_LOSS;
            velY -= 2 * dotProduct * ny * COLLISION_ENERGY_LOSS;
        }
    }
}