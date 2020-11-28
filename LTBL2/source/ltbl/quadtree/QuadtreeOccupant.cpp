#include "QuadtreeOccupant.h"

#include "QuadtreeNode.h"

#include "Quadtree.h"

#include <assert.h>

using namespace ltbl;

void QuadtreeOccupant::quadtreeUpdate() {
	if (pQuadtreeNode != nullptr)
		pQuadtreeNode->update(this);
	else {
		pQuadtree->outsideRoot.erase(this);

		pQuadtree->add(this);
	}
}

void QuadtreeOccupant::quadtreeRemove() {
	if (pQuadtreeNode != nullptr)
		pQuadtreeNode->remove(this);
	else
		pQuadtree->outsideRoot.erase(this);
}