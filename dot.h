#ifndef DOT_H
#define DOT_H

class Dot {
public:
    Dot();
    Dot(float angle, float baseRadius, int index, int totalDots);

    float getX() const { return x; }
    float getY() const { return y; }
    float getSize() const { return size; }
    float getHue() const { return hue; }

    void update(float springStiffness, float dampingFactor);
    void applyForce(float forceX, float forceY);
    void checkBoundaries();

private:
    float baseX, baseY;
    float x, y;
    float velX, velY;
    float size;
    float hue;
    float mass;

    friend class DotAnimation;
};

#endif // DOT_H