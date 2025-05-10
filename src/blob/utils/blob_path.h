#ifndef BLOBPATH_H
#define BLOBPATH_H

#include <QPainterPath>
#include <QPointF>
#include <vector>

/**
 * @brief Provides a static utility function to create a smooth QPainterPath from blob control points.
 *
 * This class is used to generate the visual outline of the blob by creating a closed
 * curve that interpolates smoothly through the given control points.
 */
class BlobPath {
public:
    /**
     * @brief Creates a closed, smooth QPainterPath using Catmull-Rom-like spline interpolation.
     *
     * Iterates through the provided control points and generates cubic Bezier segments
     * to create a smooth, closed curve passing through them. Uses neighboring points
     * to calculate control handles for the Bézier curves, effectively creating a
     * Catmull-Rom spline effect with a specified tension. Includes validation to skip
     * invalid points.
     *
     * @param control_points A vector containing the QPointF coordinates of the blob's control points.
     * @param num_of_points The number of control points in the vector. Should match control_points.size().
     * @return A QPainterPath representing the smooth outline of the blob. Returns an empty path if control_points is empty.
     */
    static QPainterPath CreateBlobPath(const std::vector<QPointF> &control_points, int num_of_points);
};

#endif // BLOBPATH_H
