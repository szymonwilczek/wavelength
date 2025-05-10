#include "blob_math.h"

#include <qmath.h>
#include <QPointF>
#include <QRandomGenerator>

bool BlobMath::IsValidPoint(const QPointF &point) {
    if (std::isnan(point.x()) || std::isnan(point.y()) ||
        std::isinf(point.x()) || std::isinf(point.y())) {
        return false;
    }

    constexpr double max_coord = 100000.0;
    if (qAbs(point.x()) > max_coord || qAbs(point.y()) > max_coord) {
        return false;
    }

    return true;
}

std::vector<QPointF> BlobMath::GenerateCircularPoints(const QPointF &center, const double radius,
                                                      const int num_of_points) {
    std::vector<QPointF> points;
    points.reserve(num_of_points);

    for (int i = 0; i < num_of_points; ++i) {
        const double angle = 2 * M_PI * i / num_of_points;

        const double random_radius = radius * (0.9 + 0.2 * QRandomGenerator::global()->generateDouble());

        QPointF point(
            center.x() + random_radius * qCos(angle),
            center.y() + random_radius * qSin(angle)
        );

        points.push_back(point);
    }

    return points;
}

QPointF BlobMath::CalculateBezierControlPoint(const QPointF &p0, const QPointF &p1, const QPointF &p2,
                                              const float tension) {
    return QPointF(
        p1.x() + tension * (p2.x() - p0.x()),
        p1.y() + tension * (p2.y() - p0.y())
    );
}
