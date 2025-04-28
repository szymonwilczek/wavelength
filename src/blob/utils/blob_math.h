#ifndef BLOBMATH_H
#define BLOBMATH_H

#include <QVector2D>
#include <QtMath>
#include <vector>

class BlobMath {
public:
    static bool isValidPoint(const QPointF &point);
    
    static double clamp(double value, double min, double max);
    
    static std::vector<QPointF> generateCircularPoints(const QPointF &center, double radius, int numPoints);
    
    static QPointF calculateBezierControlPoint(const QPointF &p0, const QPointF &p1, const QPointF &p2, float tension);
};

#endif // BLOBMATH_H