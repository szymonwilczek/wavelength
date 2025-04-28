#ifndef BLOBPATH_H
#define BLOBPATH_H

#include <QPainterPath>
#include <QPointF>
#include "blob_math.h"

class BlobPath {
public:
    static QPainterPath createBlobPath(const std::vector<QPointF>& controlPoints, int numPoints);
};

#endif // BLOBPATH_H