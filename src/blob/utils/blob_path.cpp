#include "blob_path.h"

QPainterPath BlobPath::CreateBlobPath(const std::vector<QPointF>& control_points, const int num_of_points) {
    QPainterPath path;

    if (control_points.empty()) return path;


    path.moveTo(control_points[0]);

    for (int i = 0; i < num_of_points; ++i) {
        constexpr float tension = 0.25f;
        const int prev = (i + num_of_points - 1) % num_of_points;
        const int curr = i;
        const int next = (i + 1) % num_of_points;
        const int next_next = (i + 2) % num_of_points;

        QPointF p0 = control_points[prev];
        QPointF p1 = control_points[curr];
        QPointF p2 = control_points[next];
        QPointF p3 = control_points[next_next];

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