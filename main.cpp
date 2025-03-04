#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <cmath>
#include <vector>
#include <QColor>
#include <QPointF>
#include <QDateTime>

#define _USE_MATH_DEFINES
#include <math.h>

class DotAnimation : public QWidget {
public:
    DotAnimation(QWidget *parent = nullptr) : QWidget(parent), breathePhase(0) {
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &DotAnimation::updateAnimation);
        timer->start(16); // ~60 FPS

        numDots = 25;
        dots.resize(numDots);

        springStiffness = 0.01;    // sztywność
        dampingFactor = 0.96;      // tłumienie

        for (int i = 0; i < numDots; ++i) {
            float angle = i * (2 * M_PI / numDots);
            float baseRadius = 0.7;
            dots[i].baseX = cos(angle) * baseRadius;
            dots[i].baseY = sin(angle) * baseRadius;
            dots[i].x = dots[i].baseX;
            dots[i].y = dots[i].baseY;
            dots[i].velX = 0;
            dots[i].velY = 0;
            dots[i].size = 10 + (i % 5) * 3;
            dots[i].hue = i * 360.0f / numDots;
            dots[i].mass = dots[i].size / 10.0f;
        }

        setMinimumSize(400, 400);

        lastWindowPos = QPoint(0, 0);
        lastWindowSize = QSize(0, 0);
        lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
        isWindowTracking = false;

        QTimer::singleShot(100, this, &DotAnimation::setupEventTracking);
    }

    void setupEventTracking() {
        if (window()) {
            lastWindowPos = window()->pos();
            lastWindowSize = window()->size();
            window()->installEventFilter(this);
            isWindowTracking = true;
        }
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
    if (isWindowTracking && obj == window()) {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 deltaTime = qMax(static_cast<qint64>(1), currentTime - lastUpdateTime);

        if (event->type() == QEvent::Move) {
            QPoint currentPos = window()->pos();
            QPoint delta = currentPos - lastWindowPos;

            if (!delta.isNull()) {
                float velX = -delta.x() / static_cast<float>(deltaTime) * 0.004f;
                float velY = -delta.y() / static_cast<float>(deltaTime) * 0.004f;

                applyForceToAllDots(velX, velY);
                lastWindowPos = currentPos;
                lastUpdateTime = currentTime;
            }
        }
        else if (event->type() == QEvent::Resize) {
            QSize currentSize = window()->size();
            QSize delta(currentSize.width() - lastWindowSize.width(),
                        currentSize.height() - lastWindowSize.height());

            if (delta.width() != 0 || delta.height() != 0) {
                float velX = delta.width() / static_cast<float>(deltaTime) * 0.004f;
                float velY = delta.height() / static_cast<float>(deltaTime) * 0.004f;

                for (int i = 0; i < numDots; ++i) {
                    float xFactor = 0.3f + 0.7f * std::abs(dots[i].x);
                    if ((delta.width() > 0 && dots[i].x < 0) || (delta.width() < 0 && dots[i].x > 0)) {
                        xFactor *= 0.5f;
                    }

                    float yFactor = 0.3f + 0.7f * std::abs(dots[i].y);
                    if ((delta.height() > 0 && dots[i].y < 0) || (delta.height() < 0 && dots[i].y > 0)) {
                        yFactor *= 0.5f;
                    }

                    dots[i].velX += velX * xFactor / dots[i].mass;
                    dots[i].velY += velY * yFactor / dots[i].mass;
                }

                lastWindowSize = currentSize;
                lastUpdateTime = currentTime;
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

    void applyForceToAllDots(float forceX, float forceY) {
        for (int i = 0; i < numDots; ++i) {
            float distanceFromCenter = sqrt(dots[i].x * dots[i].x + dots[i].y * dots[i].y);
            float forceFactor = 0.5f + 0.5f * distanceFromCenter;

            dots[i].velX += forceX * forceFactor / dots[i].mass;
            dots[i].velY += forceY * forceFactor / dots[i].mass;
        }
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // tło
        QLinearGradient gradient(0, 0, width(), height());
        gradient.setColorAt(0, QColor(10, 10, 25));
        gradient.setColorAt(1, QColor(30, 30, 45));
        painter.fillRect(rect(), gradient);

        int centerX = width() / 2;
        int centerY = height() / 2;
        int minDimension = std::min(width(), height()) / 2 - 40;

        for (int i = 0; i < numDots; ++i) {
            int x = centerX + minDimension * dots[i].x;
            int y = centerY + minDimension * dots[i].y;

            float breatheFactor = 0.15f * sin(breathePhase + i * 0.2f) + 0.85f;
            float size = dots[i].size * breatheFactor;

            QColor dotColor = QColor::fromHsv(
                static_cast<int>(dots[i].hue),
                220,
                200 + static_cast<int>(55 * sin(breathePhase * 0.3f + i * 0.1f))
            );

            painter.setOpacity(0.7 + 0.3 * (dots[i].y + 1.0) / 2.0);

            painter.setBrush(dotColor);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(x, y), size, size);

            painter.setBrush(Qt::white);
            painter.setOpacity(0.2);
            painter.drawEllipse(QPointF(x - size/4, y - size/4), size/5, size/5);
        }
    }

private slots:
    void updateAnimation() {
        breathePhase += 0.015f;
        if (breathePhase > 2 * M_PI) {
            breathePhase -= 2 * M_PI;
        }

        for (int i = 0; i < numDots; ++i) {
            float forceX = (dots[i].baseX - dots[i].x) * springStiffness;
            float forceY = (dots[i].baseY - dots[i].y) * springStiffness;

            // prędkość
            dots[i].velX += forceX;
            dots[i].velY += forceY;

            // tłumienie
            dots[i].velX *= dampingFactor;
            dots[i].velY *= dampingFactor;

            // pozycja
            dots[i].x += dots[i].velX;
            dots[i].y += dots[i].velY;

            float maxRadius = 0.95;
            float distanceFromCenter = sqrt(dots[i].x * dots[i].x + dots[i].y * dots[i].y);

            if (distanceFromCenter > maxRadius) {
                float normFactor = maxRadius / distanceFromCenter;
                dots[i].x *= normFactor;
                dots[i].y *= normFactor;

                float nx = dots[i].x / distanceFromCenter;
                float ny = dots[i].y / distanceFromCenter;

                float dotProduct = dots[i].velX * nx + dots[i].velY * ny;

                if (dotProduct > 0) {
                    dots[i].velX -= 2 * dotProduct * nx * 0.6;
                    dots[i].velY -= 2 * dotProduct * ny * 0.6;
                }
            }
        }

        update();
    }

private:
    struct Dot {
        float baseX, baseY;
        float x, y;
        float velX, velY;
        float size;
        float hue;
        float mass;
    };

    std::vector<Dot> dots;
    int numDots;
    float breathePhase;
    float springStiffness;
    float dampingFactor;
    QTimer *timer;
    QPoint lastWindowPos;
    QSize lastWindowSize;
    qint64 lastUpdateTime;
    bool isWindowTracking;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("PK4");

    DotAnimation *animation = new DotAnimation();
    window.setCentralWidget(animation);
    window.resize(500, 500);
    window.show();

    return app.exec();
}