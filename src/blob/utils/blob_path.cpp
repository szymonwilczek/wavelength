#include "blob_path.h"

QPainterPath BlobPath::createBlobPath(const std::vector<QPointF>& controlPoints, const int numPoints) {
    QPainterPath path;

    if (controlPoints.empty()) return path;


    path.moveTo(controlPoints[0]);

    for (int i = 0; i < numPoints; ++i) {
        constexpr float tension = 0.25f;
        const int prev = (i + numPoints - 1) % numPoints;
        const int curr = i;
        const int next = (i + 1) % numPoints;
        const int nextNext = (i + 2) % numPoints;

        QPointF p0 = controlPoints[prev];
        QPointF p1 = controlPoints[curr];
        QPointF p2 = controlPoints[next];
        QPointF p3 = controlPoints[nextNext];

        if (!BlobMath::IsValidPoint(p0) || !BlobMath::IsValidPoint(p1) ||
            !BlobMath::IsValidPoint(p2) || !BlobMath::IsValidPoint(p3)) {
            continue;
            }

        QPointF c1 = p1 + QPointF((p2.x() - p0.x()) * tension, (p2.y() - p0.y()) * tension);
        QPointF c2 = p2 + QPointF((p1.x() - p3.x()) * tension, (p1.y() - p3.y()) * tension);

        path.cubicTo(c1, c2, p2);
    }

    return path;
}