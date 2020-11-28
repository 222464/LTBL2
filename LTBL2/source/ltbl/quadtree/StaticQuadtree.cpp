#include "StaticQuadtree.h"

#include <assert.h>

using namespace ltbl;

void StaticQuadtree::add(QuadtreeOccupant* oc) {
	assert(created());

	setQuadtree(oc);

	// If the occupant fits in the root node
	if (rectContains(pRootNode->getRegion(), oc->getAABB()))
		pRootNode->add(oc);
	else
		outsideRoot.insert(oc);
}