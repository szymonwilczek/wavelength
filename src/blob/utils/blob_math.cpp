#include "blob_math.h"

bool BlobMath::isValidPoint(const QPointF &point) {
    if (std::isnan(point.x()) || std::isnan(point.y()) ||
        std::isinf(point.x()) || std::isinf(point.y())) {
        return false;
        }

    constexpr double maxCoord = 100000.0;
    if (qAbs(point.x()) > maxCoord || qAbs(point.y()) > maxCoord) {
        return false;
    }

    return true;
}

double BlobMath::clamp(const double value, const double min, const double max) {
    return qBound(min, value, max);
}

std::vector<QPointF> BlobMath::generateCircularPoints(const QPointF &center, const double radius, const int numPoints) {
    std::vector<QPointF> points;
    points.reserve(numPoints);

    for (int i = 0; i < numPoints; ++i) {
        const double angle = 2 * M_PI * i / numPoints;

        const double randomRadius = radius * (0.9 + 0.2 * (qrand() % 100) / 100.0);

        QPointF point(
            center.x() + randomRadius * qCos(angle),
            center.y() + randomRadius * qSin(angle)
        );

        points.push_back(point);
    }

    return points;
}

QPointF BlobMath::calculateBezierControlPoint(const QPointF &p0, const QPointF &p1, const QPointF &p2, const float tension) {
    return QPointF(
        p1.x() + tension * (p2.x() - p0.x()),
        p1.y() + tension * (p2.y() - p0.y())
    );
}