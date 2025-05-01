#ifndef BLOBPATH_H
#define BLOBPATH_H

#include <QPainterPath>
#include <QPointF>
#include "blob_math.h"

class BlobPath {
public:
    static QPainterPath CreateBlobPath(const std::vector<QPointF>& control_points, int num_of_points);
};

#endif // BLOBPATH_H