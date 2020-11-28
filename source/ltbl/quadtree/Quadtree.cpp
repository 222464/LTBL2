#include "Quadtree.h"

#include <algorithm>

#include <assert.h>

using namespace ltbl;

Quadtree::Quadtree()
: minNumNodeOccupants(3),
maxNumNodeOccupants(6),
maxLevels(40),
oversizeMultiplier(1.0f)
{}

void Quadtree::operator=(const Quadtree &other) {
	minNumNodeOccupants = other.minNumNodeOccupants;
	maxNumNodeOccupants = other.maxNumNodeOccupants;
	maxLevels = other.maxLevels;
	oversizeMultiplier = other.oversizeMultiplier;

	outsideRoot = other.outsideRoot;

	if (other.pRootNode != nullptr) {
		pRootNode = std::make_unique<QuadtreeNode>();

		recursiveCopy(pRootNode.get(), other.pRootNode.get(), nullptr);
	}
}

void Quadtree::setQuadtree(QuadtreeOccupant* oc) {
	oc->pQuadtree = this;
}

void Quadtree::recursiveCopy(QuadtreeNode* pThisNode, QuadtreeNode* pOtherNode, QuadtreeNode* pThisParent) {
	pThisNode->hasChildren = pOtherNode->hasChildren;
	pThisNode->level = pOtherNode->level;
	pThisNode->numOccupantsBelow = pOtherNode->numOccupantsBelow;
	pThisNode->occupants = pOtherNode->occupants;
	pThisNode->region = pOtherNode->region;

	pThisNode->pParent = pThisParent;

	pThisNode->pQuadtree = this;

	if (pThisNode->hasChildren)
	for (int i = 0; i < 4; i++) {
		pThisNode->children[i] = std::make_unique<QuadtreeNode>();

		recursiveCopy(pThisNode->children[i].get(), pOtherNode->children[i].get(), pThisNode);
	}
}

void Quadtree::pruneDeadReferences() {
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end();)
	if ((*it) == nullptr)
		it++;
	else
		it = outsideRoot.erase(it);

	if (pRootNode != nullptr)
		pRootNode->pruneDeadReferences();
}

void Quadtree::queryRegion(std::vector<QuadtreeOccupant*> &result, const sf::FloatRect &region) {
	// Query outside root elements
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++) {
		QuadtreeOccupant* oc = *it;
		if (oc != nullptr && region.intersects(oc->getAABB()))
			// Intersects, add to list
			result.push_back(oc);
	}

	std::list<QuadtreeNode*> open;

	open.push_back(pRootNode.get());

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		if (region.intersects(pCurrent->region)) {
			for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++) {
				QuadtreeOccupant* oc = *it;

				if (oc != nullptr && region.intersects(oc->getAABB()))
					// Visible, add to list
					result.push_back(oc);
			}

			// Add children to open list if they intersect the region
			if (pCurrent->hasChildren)
			for (int i = 0; i < 4; i++)
			if (pCurrent->children[i]->getNumOccupantsBelow() != 0)
				open.push_back(pCurrent->children[i].get());
		}
	}
}

void Quadtree::queryPoint(std::vector<QuadtreeOccupant*> &result, const sf::Vector2f &p) {
	// Query outside root elements
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++) {
		QuadtreeOccupant* oc = *it;

		if (oc != nullptr && oc->getAABB().contains(p))
			// Intersects, add to list
			result.push_back(oc);
	}

	std::list<QuadtreeNode*> open;

	open.push_back(pRootNode.get());

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		if (pCurrent->region.contains(p)) {
			for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++) {
				QuadtreeOccupant* oc = *it;

				if (oc != nullptr && oc->getAABB().contains(p))
					// Visible, add to list
					result.push_back(oc);
			}

			// Add children to open list if they intersect the region
			if (pCurrent->hasChildren)
			for (int i = 0; i < 4; i++)
			if (pCurrent->children[i]->getNumOccupantsBelow() != 0)
				open.push_back(pCurrent->children[i].get());
		}
	}
}

void Quadtree::queryShape(std::vector<QuadtreeOccupant*> &result, const sf::ConvexShape &shape) {
	// Query outside root elements
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = outsideRoot.begin(); it != outsideRoot.end(); it++) {
		QuadtreeOccupant* oc = *it;

		if (oc != nullptr && shapeIntersection(shapeFromRect(oc->getAABB()), shape))
			// Intersects, add to list
			result.push_back(oc);
	}

	std::list<QuadtreeNode*> open;

	open.push_back(pRootNode.get());

	while (!open.empty()) {
		// Depth-first (results in less memory usage), remove objects from open list
		QuadtreeNode* pCurrent = open.back();
		open.pop_back();

		if (shapeIntersection(shapeFromRect(pCurrent->region), shape)) {
			for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++) {
				QuadtreeOccupant* oc = *it;
				sf::ConvexShape r = shapeFromRect(oc->getAABB());

				if (oc != nullptr && shapeIntersection(shapeFromRect(oc->getAABB()), shape))
					// Visible, add to list
					result.push_back(oc);
			}

			// Add children to open list if they intersect the region
			if (pCurrent->hasChildren)
			for (int i = 0; i < 4; i++)
			if (pCurrent->children[i]->getNumOccupantsBelow() != 0)
				open.push_back(pCurrent->children[i].get());
		}
	}
}
