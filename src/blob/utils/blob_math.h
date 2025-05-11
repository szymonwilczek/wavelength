#ifndef BLOBMATH_H
#define BLOBMATH_H

#include <concepts>
#include <vector>

class QPointF;

/**
 * @brief Provides static utility functions for mathematical operations related to the Blob animation.
 *
 * This class contains helper functions for tasks such as validating points, clamping values,
 * generating points on a circle, and calculating Bezier control points (potentially for path smoothing).
 */
class BlobMath {
public:
    /**
     * @brief Checks if a QPointF contains valid coordinate values.
     * Verifies that coordinates are not NaN, infinite, or excessively large.
     * @param point The QPointF to validate.
     * @return True if the point is valid, false otherwise.
     */
    static bool IsValidPoint(const QPointF &point);

    /**
     * @brief Clamps a value between a minimum and maximum limit.
     * Requires the type T to be totally ordered (support <, >, <=, >=, ==, !=).
     * Uses Qt's qBound internally.
     * @tparam T The type of the value to clamp. Must satisfy std::totally_ordered.
     * @param value The value to clamp.
     * @param min The minimum allowed value.
     * @param max The maximum allowed value.
     * @return The clamped value, guaranteed to be within [min, max].
     */
    template<typename T>
        requires std::totally_ordered<T>
    static T Clamp(const T &value, const T &min, const T &max) {
        return qBound(min, value, max);
    }

    /**
     * @brief Generates a specified number of points approximately distributed on a circle.
     * Introduces slight randomness to the radius of each point for a more organic look.
     * @param center The center of the circle.
     * @param radius The base radius of the circle.
     * @param num_of_points The number of points to generate.
     * @return A std::vector<QPointF> containing the generated points.
     */
    static std::vector<QPointF> GenerateCircularPoints(const QPointF &center, double radius, int num_of_points);

    /**
     * @brief Calculates a control point for a cubic Bezier segment, often used in Catmull-Rom splines.
     * This helps create smooth curves that pass through a series of points (p0, p1, p2, ...).
     * The calculated point is a control point associated with the segment passing through p1.
     * @param p0 The point before the segment's start point.
     * @param p1 The start point of the curve segment.
     * @param p2 The end point of the curve segment.
     * @param tension Controls the "tightness" of the curve. Typically, 0.5 for Catmull-Rom.
     * @return The calculated control point QPointF.
     */
    static QPointF CalculateBezierControlPoint(const QPointF &p0, const QPointF &p1, const QPointF &p2, float tension);
};

#endif // BLOBMATH_H
