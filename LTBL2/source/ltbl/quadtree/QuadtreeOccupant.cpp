#include <ltbl/quadtree/QuadtreeOccupant.h>

#include <ltbl/quadtree/QuadtreeNode.h>

#include <ltbl/quadtree/Quadtree.h>

#include <assert.h>

using namespace ltbl;

void QuadtreeOccupant::quadtreeUpdate() {
	if (_pQuadtreeNode != nullptr)
		_pQuadtreeNode->update(this);
	else {
		_pQuadtree->_outsideRoot.erase(this);

		_pQuadtree->add(this);
	}
}

void QuadtreeOccupant::quadtreeRemove() {
	if (_pQuadtreeNode != nullptr)
		_pQuadtreeNode->remove(this);
	else
		_pQuadtree->_outsideRoot.erase(this);
}