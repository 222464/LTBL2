#include <ltbl/quadtree/DynamicQuadtree.h>

#include <assert.h>

using namespace ltbl;

void DynamicQuadtree::operator=(const DynamicQuadtree &other) {
	Quadtree::operator=(other);

	_minOutsideRoot = other._minOutsideRoot;
	_maxOutsideRoot = other._maxOutsideRoot;
}

void DynamicQuadtree::add(QuadtreeOccupant* oc) {
	assert(created());

	// If the occupant fits in the root node
	if (rectContains(_pRootNode->getRegion(), oc->getAABB()))
		_pRootNode->add(oc);
	else
		_outsideRoot.insert(oc);

	setQuadtree(oc);
}

void DynamicQuadtree::expand() {
	// Find direction with most occupants
	sf::Vector2f averageDir(0.0f, 0.0f);

	for (std::unordered_set<QuadtreeOccupant*>::iterator it = _outsideRoot.begin(); it != _outsideRoot.end(); it++)
		averageDir += vectorNormalize(rectCenter((*it)->getAABB()) - rectCenter(_pRootNode->getRegion()));

	sf::Vector2f centerOffsetDist(rectHalfDims(_pRootNode->getRegion()) / _oversizeMultiplier);

	sf::Vector2f centerOffset((averageDir.x > 0.0f ? 1.0f : -1.0f) * centerOffsetDist.x, (averageDir.y > 0.0f ? 1.0f : -1.0f) * centerOffsetDist.y);

	// Child node position of current root node
	int rX = centerOffset.x > 0.0f ? 0 : 1;
	int rY = centerOffset.y > 0.0f ? 0 : 1;

	sf::FloatRect newRootAABB = rectFromBounds(sf::Vector2f(0.0f, 0.0f), centerOffsetDist * 4.0f);

	newRootAABB = rectRecenter(newRootAABB, centerOffset + rectCenter(_pRootNode->getRegion()));

	QuadtreeNode* pNewRoot = new QuadtreeNode(newRootAABB,  _pRootNode->_level + 1, nullptr, this);

	// ----------------------- Manual Children Creation for New Root -------------------------

	sf::Vector2f halfRegionDims = rectHalfDims(pNewRoot->_region);
	sf::Vector2f regionLowerBound = rectLowerBound(pNewRoot->_region);
	sf::Vector2f regionCenter = rectCenter(pNewRoot->_region);

	// Create the children nodes
	for(int x = 0; x < 2; x++)
		for(int y = 0; y < 2; y++) {
			if(x == rX && y == rY)
				pNewRoot->_children[x + y * 2].reset(_pRootNode.release());
			else {
				sf::Vector2f offset(x * halfRegionDims.x, y * halfRegionDims.y);

				sf::FloatRect childAABB = rectFromBounds(regionLowerBound + offset, regionCenter + offset);

				// Scale up AABB by the oversize multiplier
				sf::Vector2f center = rectCenter(childAABB);
					
				childAABB.width *= _oversizeMultiplier;
				childAABB.height *= _oversizeMultiplier;

				childAABB = rectRecenter(childAABB, center);
	
				pNewRoot->_children[x + y * 2].reset(new QuadtreeNode(childAABB, _pRootNode->_level, pNewRoot, this));
			}
		}

	pNewRoot->_hasChildren = true;
	pNewRoot->_numOccupantsBelow = _pRootNode->_numOccupantsBelow;
	_pRootNode->_pParent = pNewRoot;

	// Transfer ownership
	_pRootNode.release();
	_pRootNode.reset(pNewRoot);

	// ----------------------- Try to Add Previously Outside Root -------------------------

	// Make copy so don't try to re-add ones just added
	std::unordered_set<QuadtreeOccupant*> outsideRootCopy(_outsideRoot);
	_outsideRoot.clear();

	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRootCopy.begin(); it != outsideRootCopy.end(); it++)
		add(*it);
}

void DynamicQuadtree::contract() {
	assert(_pRootNode->_hasChildren);

	// Find child with the most occupants and shrink to that
	int maxIndex = 0;

	for (int i = 1; i < 4; i++)
	if (_pRootNode->_children[i]->getNumOccupantsBelow() >
		_pRootNode->_children[maxIndex]->getNumOccupantsBelow())
		maxIndex = i;

	// Reorganize
	for (int i = 0; i < 4; i++) {
		if (i == maxIndex)
			continue;

		_pRootNode->_children[i]->removeForDeletion(_outsideRoot);
	}

	QuadtreeNode* pNewRoot = _pRootNode->_children[maxIndex].release();

	_pRootNode->destroyChildren();

	_pRootNode->removeForDeletion(_outsideRoot);

	_pRootNode.reset(pNewRoot);

	_pRootNode->_pParent = nullptr;
}

void DynamicQuadtree::trim() {
	if(_pRootNode.get() == nullptr)
		return;

	// Check if should grow
	if(_outsideRoot.size() > _maxOutsideRoot)
		expand();
	else if(_outsideRoot.size() < _minOutsideRoot && _pRootNode->_hasChildren)
		contract();
}