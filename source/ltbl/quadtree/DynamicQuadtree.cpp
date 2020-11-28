#include "DynamicQuadtree.h"

#include <assert.h>

using namespace ltbl;

void DynamicQuadtree::operator=(const DynamicQuadtree &other) {
	Quadtree::operator=(other);

	minOutsideRoot = other.minOutsideRoot;
	maxOutsideRoot = other.maxOutsideRoot;
}

void DynamicQuadtree::add(QuadtreeOccupant* oc) {
	assert(created());

	// If the occupant fits in the root node
	if (rectContains(pRootNode->getRegion(), oc->getAABB()))
		pRootNode->add(oc);
	else
		outsideRoot.insert(oc);

	setQuadtree(oc);
}

void DynamicQuadtree::expand() {
	// Find direction with most occupants
	sf::Vector2f averageDir(0.0f, 0.0f);

	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++)
		averageDir += vectorNormalize(rectCenter((*it)->getAABB()) - rectCenter(pRootNode->getRegion()));

	sf::Vector2f centerOffsetDist(rectHalfDims(pRootNode->getRegion()) / oversizeMultiplier);

	sf::Vector2f centerOffset((averageDir.x > 0.0f ? 1.0f : -1.0f) * centerOffsetDist.x, (averageDir.y > 0.0f ? 1.0f : -1.0f) * centerOffsetDist.y);

	// Child node position of current root node
	int rX = centerOffset.x > 0.0f ? 0 : 1;
	int rY = centerOffset.y > 0.0f ? 0 : 1;

	sf::FloatRect newRootAABB = rectFromBounds(sf::Vector2f(0.0f, 0.0f), centerOffsetDist * 4.0f);

	newRootAABB = rectRecenter(newRootAABB, centerOffset + rectCenter(pRootNode->getRegion()));

        std::unique_ptr<QuadtreeNode> pNewRoot = std::make_unique<QuadtreeNode>(newRootAABB,  pRootNode->level + 1, nullptr, this);

	// ----------------------- Manual Children Creation for New Root -------------------------

	sf::Vector2f halfRegionDims = rectHalfDims(pNewRoot->region);
	sf::Vector2f regionLowerBound = rectLowerBound(pNewRoot->region);
	sf::Vector2f regionCenter = rectCenter(pNewRoot->region);

	// Create the children nodes
	for(int x = 0; x < 2; x++)
		for(int y = 0; y < 2; y++) {
			if(x == rX && y == rY)
				pNewRoot->children[x + y * 2].reset(pRootNode.release());
			else {
				sf::Vector2f offset(x * halfRegionDims.x, y * halfRegionDims.y);

				sf::FloatRect childAABB = rectFromBounds(regionLowerBound + offset, regionCenter + offset);

				// Scale up AABB by the oversize multiplier
				sf::Vector2f center = rectCenter(childAABB);
					
				childAABB.width *= oversizeMultiplier;
				childAABB.height *= oversizeMultiplier;

				childAABB = rectRecenter(childAABB, center);
	
				pNewRoot->children[x + y * 2] = std::make_unique<QuadtreeNode>(childAABB, pRootNode->level, pNewRoot.get(), this);
			}
		}

	pNewRoot->hasChildren = true;
	pNewRoot->numOccupantsBelow = pRootNode->numOccupantsBelow;
	pRootNode->pParent = pNewRoot.get();

	// Transfer ownership
	pRootNode = std::move(pNewRoot);

	// ----------------------- Try to Add Previously Outside Root -------------------------

	// Make copy so don't try to re-add ones just added
	std::unordered_set<QuadtreeOccupant*> outsideRootCopy(outsideRoot);
	outsideRoot.clear();

	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRootCopy.begin(); it != outsideRootCopy.end(); it++)
		add(*it);
}

void DynamicQuadtree::contract() {
	assert(pRootNode->hasChildren);

	// Find child with the most occupants and shrink to that
	int maxIndex = 0;

	for (int i = 1; i < 4; i++)
	if (pRootNode->children[i]->getNumOccupantsBelow() >
		pRootNode->children[maxIndex]->getNumOccupantsBelow())
		maxIndex = i;

	// Reorganize
	for (int i = 0; i < 4; i++) {
		if (i == maxIndex)
			continue;

		pRootNode->children[i]->removeForDeletion(outsideRoot);
	}

	QuadtreeNode* pNewRoot = pRootNode->children[maxIndex].release();

	pRootNode->destroyChildren();

	pRootNode->removeForDeletion(outsideRoot);

	pRootNode.reset(pNewRoot);

	pRootNode->pParent = nullptr;
}

void DynamicQuadtree::trim() {
	if(pRootNode.get() == nullptr)
		return;

	// Check if should grow
	if(outsideRoot.size() > maxOutsideRoot)
		expand();
	else if(outsideRoot.size() < minOutsideRoot && pRootNode->hasChildren)
		contract();
}
