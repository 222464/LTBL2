#include <ltbl/quadtree/Quadtree.h>

#include <algorithm>

#include <assert.h>

using namespace ltbl;

Quadtree::Quadtree()
: _minNumNodeOccupants(3),
_maxNumNodeOccupants(6),
_maxLevels(40),
_oversizeMultiplier(1.0f)
{}

void Quadtree::operator=(const Quadtree &other) {
	_minNumNodeOccupants = other._minNumNodeOccupants;
	_maxNumNodeOccupants = other._maxNumNodeOccupants;
	_maxLevels = other._maxLevels;
	_oversizeMultiplier = other._oversizeMultiplier;

	_outsideRoot = other._outsideRoot;

	if (other._pRootNode != nullptr) {
		_pRootNode.reset(new QuadtreeNode());

		recursiveCopy(_pRootNode.get(), other._pRootNode.get(), nullptr);
	}
}

void Quadtree::setQuadtree(QuadtreeOccupant* oc) {
	oc->_pQuadtree = this;
}

void Quadtree::recursiveCopy(QuadtreeNode* pThisNode, QuadtreeNode* pOtherNode, QuadtreeNode* pThisParent) {
	pThisNode->_hasChildren = pOtherNode->_hasChildren;
	pThisNode->_level = pOtherNode->_level;
	pThisNode->_numOccupantsBelow = pOtherNode->_numOccupantsBelow;
	pThisNode->_occupants = pOtherNode->_occupants;
	pThisNode->_region = pOtherNode->_region;

	pThisNode->_pParent = pThisParent;

	pThisNode->_pQuadtree = this;

	if (pThisNode->_hasChildren)
	for (int i = 0; i < 4; i++) {
		pThisNode->_children[i].reset(new QuadtreeNode());

		recursiveCopy(pThisNode->_children[i].get(), pOtherNode->_children[i].get(), pThisNode);
	}
}

void Quadtree::pruneDeadReferences() {
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = _outsideRoot.begin(); it != _outsideRoot.end();)
	if ((*it) == nullptr)
		it++;
	else
		it = _outsideRoot.erase(it);

	if (_pRootNode != nullptr)
		_pRootNode->pruneDeadReferences();
}

void Quadtree::queryRegion(std::vector<QuadtreeOccupant*> &result, const sf::FloatRect &region) {
	// Query outside root elements
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = _outsideRoot.begin(); it != _outsideRoot.end(); it++) {
		QuadtreeOccupant* oc = *it;
		if (oc != nullptr && region.intersects(oc->getAABB()))
			// Intersects, add to list
			result.push_back(oc);
	}

	std::list<QuadtreeNode*> open;

	open.push_back(_pRootNode.get());

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		if (region.intersects(pCurrent->_region)) {
			for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++) {
				QuadtreeOccupant* oc = *it;

				if (oc != nullptr && region.intersects(oc->getAABB()))
					// Visible, add to list
					result.push_back(oc);
			}

			// Add children to open list if they intersect the region
			if (pCurrent->_hasChildren)
			for (int i = 0; i < 4; i++)
			if (pCurrent->_children[i]->getNumOccupantsBelow() != 0)
				open.push_back(pCurrent->_children[i].get());
		}
	}
}

void Quadtree::queryPoint(std::vector<QuadtreeOccupant*> &result, const sf::Vector2f &p) {
	// Query outside root elements
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = _outsideRoot.begin(); it != _outsideRoot.end(); it++) {
		QuadtreeOccupant* oc = *it;

		if (oc != nullptr && oc->getAABB().contains(p))
			// Intersects, add to list
			result.push_back(oc);
	}

	std::list<QuadtreeNode*> open;

	open.push_back(_pRootNode.get());

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		if (pCurrent->_region.contains(p)) {
			for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++) {
				QuadtreeOccupant* oc = *it;

				if (oc != nullptr && oc->getAABB().contains(p))
					// Visible, add to list
					result.push_back(oc);
			}

			// Add children to open list if they intersect the region
			if (pCurrent->_hasChildren)
			for (int i = 0; i < 4; i++)
			if (pCurrent->_children[i]->getNumOccupantsBelow() != 0)
				open.push_back(pCurrent->_children[i].get());
		}
	}
}

void Quadtree::queryShape(std::vector<QuadtreeOccupant*> &result, const sf::ConvexShape &shape) {
	// Query outside root elements
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = _outsideRoot.begin(); it != _outsideRoot.end(); it++) {
		QuadtreeOccupant* oc = *it;

		if (oc != nullptr && shapeIntersection(shapeFromRect(oc->getAABB()), shape))
			// Intersects, add to list
			result.push_back(oc);
	}

	std::list<QuadtreeNode*> open;

	open.push_back(_pRootNode.get());

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		if (shapeIntersection(shapeFromRect(pCurrent->_region), shape)) {
			for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->_occupants.begin(); it != pCurrent->_occupants.end(); it++) {
				QuadtreeOccupant* oc = *it;
				sf::ConvexShape r = shapeFromRect(oc->getAABB());

				if (oc != nullptr && shapeIntersection(shapeFromRect(oc->getAABB()), shape))
					// Visible, add to list
					result.push_back(oc);
			}

			// Add children to open list if they intersect the region
			if (pCurrent->_hasChildren)
			for (int i = 0; i < 4; i++)
			if (pCurrent->_children[i]->getNumOccupantsBelow() != 0)
				open.push_back(pCurrent->_children[i].get());
		}
	}
}
