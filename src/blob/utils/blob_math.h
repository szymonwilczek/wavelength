#ifndef BLOBMATH_H
#define BLOBMATH_H

#include <QVector2D>
#include <QtMath>
#include <vector>
#include <concepts>

class BlobMath {
public:
    static bool IsValidPoint(const QPointF &point);

    template<typename T>
    requires std::totally_ordered<T>
    static T Clamp(const T& value, const T& min, const T& max) {
        return qBound(min, value, max);
    }

    static std::vector<QPointF> GenerateCircularPoints(const QPointF &center, double radius, int num_of_points);

    static QPointF CalculateBezierControlPoint(const QPointF &p0, const QPointF &p1, const QPointF &p2, float tension);
};

#endif // BLOBMATH_H