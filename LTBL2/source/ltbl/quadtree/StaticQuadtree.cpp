#include <ltbl/quadtree/StaticQuadtree.h>

#include <assert.h>

using namespace ltbl;

void StaticQuadtree::add(QuadtreeOccupant* oc) {
	assert(created());

	setQuadtree(oc);

	// If the occupant fits in the root node
	if (rectContains(_pRootNode->getRegion(), oc->getAABB()))
		_pRootNode->add(oc);
	else
		_outsideRoot.insert(oc);
}