#include <ltbl2/quadtree/QuadtreeOccupant.h>
#include <ltbl2/quadtree/QuadtreeNode.h>

#include <assert.h>

using namespace ltbl;

void QuadtreeOccupant::quadtreeUpdate() {
    if (_pQuadtreeNode != nullptr)
        _pQuadtreeNode->update(this);
}

void QuadtreeOccupant::quadtreeRemove() {
    if (_pQuadtreeNode != nullptr)
        _pQuadtreeNode->remove(this);
}