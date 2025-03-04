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

    
    float getVelocityX() const { return velX; }
    float getVelocityY() const { return velY; }

    void setPosition(float newX, float newY) {
        x = newX;
        y = newY;
        baseX = newX;
        baseY = newY;
        velX = 0;
        velY = 0;
    }

    void setSize(float newSize) {
        size = newSize;
        mass = size / 10.0f;
    }

    
    void stopMotion();

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

#endif 