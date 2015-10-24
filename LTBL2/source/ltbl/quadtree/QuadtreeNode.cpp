#include <ltbl/quadtree/QuadtreeNode.h>

#include <ltbl/quadtree/Quadtree.h>

#include <assert.h>

using namespace ltbl;

QuadtreeNode::QuadtreeNode(const sf::FloatRect &region, int level, QuadtreeNode* pParent, Quadtree* pQuadtree)
:  _pParent(pParent), _pQuadtree(pQuadtree), _hasChildren(false),  _region(region), _level(level),
_numOccupantsBelow(0)
{}

void QuadtreeNode::create(const sf::FloatRect &region, int level, QuadtreeNode* pParent, Quadtree* pQuadtree) {
	_hasChildren = false;

	_region = region;
	_level = level;
	_pParent = pParent;
	_pQuadtree = pQuadtree;
}

void QuadtreeNode::getPossibleOccupantPosition(QuadtreeOccupant* oc, sf::Vector2i &point) {
	// Compare the center of the AABB of the occupant to that of this node to determine
	// which child it may (possibly, not certainly) fit in
	const sf::Vector2f &occupantCenter = rectCenter(oc->getAABB());
	const sf::Vector2f &nodeRegionCenter = rectCenter(_region);

	point.x = occupantCenter.x > nodeRegionCenter.x ? 1 : 0;
	point.y = occupantCenter.y > nodeRegionCenter.y ? 1 : 0;
}

void QuadtreeNode::addToThisLevel(QuadtreeOccupant* oc) {
	oc->_pQuadtreeNode = this;

	if (_occupants.find(oc) != _occupants.end())
		return;

	_occupants.insert(oc);
}

bool QuadtreeNode::addToChildren(QuadtreeOccupant* oc) {
	assert(_hasChildren);

	sf::Vector2i position;

	getPossibleOccupantPosition(oc, position);

	QuadtreeNode* pChild = _children[position.x + position.y * 2].get();

	// See if the occupant fits in the child at the selected position
	if (rectContains(pChild->_region, oc->getAABB())) {
		// Fits, so can add to the child and finish
		pChild->add(oc);

		return true;
	}

	return false;
}

void QuadtreeNode::partition() {
	assert(!_hasChildren);

	sf::Vector2f halfRegionDims = rectHalfDims(_region);
	sf::Vector2f regionLowerBound = rectLowerBound(_region);
	sf::Vector2f regionCenter = rectCenter(_region);

	int nextLowerLevel = _level - 1;

	for (int x = 0; x < 2; x++)
	for (int y = 0; y < 2; y++) {
		sf::Vector2f offset(x * halfRegionDims.x, y * halfRegionDims.y);

		sf::FloatRect childAABB = rectFromBounds(regionLowerBound + offset, regionCenter + offset);

		// Scale up AABB by the oversize multiplier
		sf::Vector2f newHalfDims = rectHalfDims(childAABB);
		sf::Vector2f center = rectCenter(childAABB);
		childAABB = rectFromBounds(center - newHalfDims, center + newHalfDims);

		_children[x + y * 2].reset(new QuadtreeNode(childAABB, nextLowerLevel, this, _pQuadtree));
	}

	_hasChildren = true;
}

void QuadtreeNode::merge() {
	if (_hasChildren) {
		// Place all occupants at lower levels into this node
		getOccupants(_occupants);

		destroyChildren();
	}
}

void QuadtreeNode::getOccupants(std::unordered_set<QuadtreeOccupant*> &occupants) {
	// Iteratively parse subnodes in order to collect all occupants below this node
	std::list<QuadtreeNode*> open;

	open.push_back(this);

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		// Get occupants
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++)
		if ((*it) != nullptr) {
			// Assign new node
			(*it)->_pQuadtreeNode = this;

			// Add to this node
			occupants.insert(*it);
		}

		// If the node has children, add them to the open list
		if (pCurrent->_hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->_children[i].get());
	}
}

void QuadtreeNode::removeForDeletion(std::unordered_set<QuadtreeOccupant*> &occupants) {
	// Iteratively parse subnodes in order to collect all occupants below this node
	std::list<QuadtreeNode*> open;

	open.push_back(this);

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		// Get occupants
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++)
		if ((*it) != nullptr) {
			// Since will be deleted, remove the reference
			(*it)->_pQuadtreeNode = nullptr;

			// Add to this node
			occupants.insert(*it);
		}

		// If the node has children, add them to the open list
		if (pCurrent->_hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->_children[i].get());
	}
}

void QuadtreeNode::getAllOccupantsBelow(std::vector<QuadtreeOccupant*> &occupants) {
	// Iteratively parse subnodes in order to collect all occupants below this node
	std::list<QuadtreeNode*> open;

	open.push_back(this);

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		// Get occupants
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++)
		if ((*it) != nullptr)
			// Add to this node
			occupants.push_back(*it);

		// If the node has children, add them to the open list
		if (pCurrent->_hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->_children[i].get());
	}
}

void QuadtreeNode::getAllOccupantsBelow(std::unordered_set<QuadtreeOccupant*> &occupants) {
	// Iteratively parse subnodes in order to collect all occupants below this node
	std::list<QuadtreeNode*> open;

	open.push_back(this);

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		// Get occupants
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++)
		if ((*it) == nullptr)
			// Add to this node
			occupants.insert(*it);

		// If the node has children, add them to the open list
		if (pCurrent->_hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->_children[i].get());
	}
}

void QuadtreeNode::update(QuadtreeOccupant* oc) {
	if (oc == nullptr)
		return;

	if (!_occupants.empty())
		// Remove, may be re-added to this node later
		_occupants.erase(oc);

	// Propogate upwards, looking for a node that has room (the current one may still have room)
	QuadtreeNode* pNode = this;

	while (pNode != nullptr) {
		pNode->_numOccupantsBelow--;

		// If has room for 1 more, found a spot
		if (rectContains(pNode->_region, oc->getAABB()))
			break;

		pNode = pNode->_pParent;
	}

	// If no node that could contain the occupant was found, add to outside root set
	if (pNode == nullptr) {
		assert(_pQuadtree != nullptr);

		if (_pQuadtree->_outsideRoot.find(oc) != _pQuadtree->_outsideRoot.end())
			return;

		_pQuadtree->_outsideRoot.insert(oc);

		oc->_pQuadtreeNode = nullptr;
	}
	else // Add to the selected node
		pNode->add(oc);
}

void QuadtreeNode::remove(QuadtreeOccupant* oc) {
	assert(!_occupants.empty());

	// Remove from node
	_occupants.erase(oc);

	if (oc == nullptr)
		return;

	// Propogate upwards, merging if there are enough occupants in the node
	QuadtreeNode* pNode = this;

	while (pNode != nullptr) {
		pNode->_numOccupantsBelow--;

		if (pNode->_numOccupantsBelow >= _pQuadtree->_minNumNodeOccupants) {
			pNode->merge();

			break;
		}

		pNode = pNode->_pParent;
	}
}

void QuadtreeNode::add(QuadtreeOccupant* oc) {
	assert(oc != nullptr);

	_numOccupantsBelow++;

	// See if the occupant fits into any children (if there are any)
	if (_hasChildren) {
		if (addToChildren(oc))
			return; // Fit, can stop
	}
	else {
		// Check if we need a new partition
		if (_occupants.size() >= _pQuadtree->_maxNumNodeOccupants && _level < _pQuadtree->_maxLevels) {
			partition();

			if (addToChildren(oc))
				return;
		}
	}

	// Did not fit in anywhere, add to this level, even if it goes over the maximum size
	addToThisLevel(oc);
}

void QuadtreeNode::pruneDeadReferences() {
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = _occupants.begin(); it != _occupants.end();) {
		if ((*it) == nullptr)
			it++;
		else
			it = _occupants.erase(it);
	}

	if (_hasChildren)
	for (int i = 0; i < 4; i++)
		_children[i]->pruneDeadReferences();
}
