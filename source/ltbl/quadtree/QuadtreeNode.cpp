#include "QuadtreeNode.h"

#include "Quadtree.h"

#include <assert.h>

using namespace ltbl;

QuadtreeNode::QuadtreeNode(const sf::FloatRect &region, int level, QuadtreeNode* pParent, Quadtree* pQuadtree)
:  pParent(pParent), pQuadtree(pQuadtree), hasChildren(false),  region(region), level(level),
numOccupantsBelow(0)
{}

void QuadtreeNode::create(const sf::FloatRect &region, int level, QuadtreeNode* pParent, Quadtree* pQuadtree) {
	hasChildren = false;

	this->region = region;
	this->level = level;
	this->pParent = pParent;
	this->pQuadtree = pQuadtree;
}

void QuadtreeNode::getPossibleOccupantPosition(QuadtreeOccupant* oc, sf::Vector2i &point) {
	// Compare the center of the AABB of the occupant to that of this node to determine
	// which child it may (possibly, not certainly) fit in
	const sf::Vector2f &occupantCenter = rectCenter(oc->getAABB());
	const sf::Vector2f &nodeRegionCenter = rectCenter(region);

	point.x = occupantCenter.x > nodeRegionCenter.x ? 1 : 0;
	point.y = occupantCenter.y > nodeRegionCenter.y ? 1 : 0;
}

void QuadtreeNode::addToThisLevel(QuadtreeOccupant* oc) {
	oc->pQuadtreeNode = this;

	if (occupants.find(oc) != occupants.end())
		return;

	occupants.insert(oc);
}

bool QuadtreeNode::addToChildren(QuadtreeOccupant* oc) {
	assert(hasChildren);

	sf::Vector2i position;

	getPossibleOccupantPosition(oc, position);

	QuadtreeNode* pChild = children[position.x + position.y * 2].get();

	// See if the occupant fits in the child at the selected position
	if (rectContains(pChild->region, oc->getAABB())) {
		// Fits, so can add to the child and finish
		pChild->add(oc);

		return true;
	}

	return false;
}

void QuadtreeNode::partition() {
	assert(!hasChildren);

	sf::Vector2f halfRegionDims = rectHalfDims(region);
	sf::Vector2f regionLowerBound = rectLowerBound(region);
	sf::Vector2f regionCenter = rectCenter(region);

	int nextLowerLevel = level - 1;

	for (int x = 0; x < 2; x++)
	for (int y = 0; y < 2; y++) {
		sf::Vector2f offset(x * halfRegionDims.x, y * halfRegionDims.y);

		sf::FloatRect childAABB = rectFromBounds(regionLowerBound + offset, regionCenter + offset);

		// Scale up AABB by the oversize multiplier
		sf::Vector2f newHalfDims = rectHalfDims(childAABB);
		sf::Vector2f center = rectCenter(childAABB);
		childAABB = rectFromBounds(center - newHalfDims, center + newHalfDims);

		children[x + y * 2] = std::make_unique<QuadtreeNode>(childAABB, nextLowerLevel, this, pQuadtree);
	}

	hasChildren = true;
}

void QuadtreeNode::merge() {
	if (hasChildren) {
		// Place all occupants at lower levels into this node
		getOccupants(occupants);

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
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++)
		if ((*it) != nullptr) {
			// Assign new node
			(*it)->pQuadtreeNode = this;

			// Add to this node
			occupants.insert(*it);
		}

		// If the node has children, add them to the open list
		if (pCurrent->hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->children[i].get());
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
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++)
		if ((*it) != nullptr) {
			// Since will be deleted, remove the reference
			(*it)->pQuadtreeNode = nullptr;

			// Add to this node
			occupants.insert(*it);
		}

		// If the node has children, add them to the open list
		if (pCurrent->hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->children[i].get());
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
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++)
		if ((*it) != nullptr)
			// Add to this node
			occupants.push_back(*it);

		// If the node has children, add them to the open list
		if (pCurrent->hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->children[i].get());
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
		for (std::unordered_set<QuadtreeOccupant*>::iterator it = pCurrent->occupants.begin(); it != pCurrent->occupants.end(); it++)
		if ((*it) == nullptr)
			// Add to this node
			occupants.insert(*it);

		// If the node has children, add them to the open list
		if (pCurrent->hasChildren)
		for (int i = 0; i < 4; i++)
			open.push_back(pCurrent->children[i].get());
	}
}

void QuadtreeNode::update(QuadtreeOccupant* oc) {
	if (oc == nullptr)
		return;

	if (!occupants.empty())
		// Remove, may be re-added to this node later
		occupants.erase(oc);

	// Propogate upwards, looking for a node that has room (the current one may still have room)
	QuadtreeNode* pNode = this;

	while (pNode != nullptr) {
		pNode->numOccupantsBelow--;

		// If has room for 1 more, found a spot
		if (rectContains(pNode->region, oc->getAABB()))
			break;

		pNode = pNode->pParent;
	}

	// If no node that could contain the occupant was found, add to outside root set
	if (pNode == nullptr) {
		assert(pQuadtree != nullptr);

		if (pQuadtree->outsideRoot.find(oc) != pQuadtree->outsideRoot.end())
			return;

		pQuadtree->outsideRoot.insert(oc);

		oc->pQuadtreeNode = nullptr;
	}
	else // Add to the selected node
		pNode->add(oc);
}

void QuadtreeNode::remove(QuadtreeOccupant* oc) {
	assert(!occupants.empty());

	// Remove from node
	occupants.erase(oc);

	if (oc == nullptr)
		return;

	// Propogate upwards, merging if there are enough occupants in the node
	QuadtreeNode* pNode = this;

	while (pNode != nullptr) {
		pNode->numOccupantsBelow--;

		if (pNode->numOccupantsBelow >= pQuadtree->minNumNodeOccupants) {
			pNode->merge();

			break;
		}

		pNode = pNode->pParent;
	}
}

void QuadtreeNode::add(QuadtreeOccupant* oc) {
	assert(oc != nullptr);

	numOccupantsBelow++;

	// See if the occupant fits into any children (if there are any)
	if (hasChildren) {
		if (addToChildren(oc))
			return; // Fit, can stop
	}
	else {
		// Check if we need a new partition
		if (occupants.size() >= pQuadtree->maxNumNodeOccupants && level < pQuadtree->maxLevels) {
			partition();

			if (addToChildren(oc))
				return;
		}
	}

	// Did not fit in anywhere, add to this level, even if it goes over the maximum size
	addToThisLevel(oc);
}

void QuadtreeNode::pruneDeadReferences() {
	for (std::unordered_set<QuadtreeOccupant*>::iterator it = occupants.begin(); it != occupants.end();) {
		if ((*it) == nullptr)
			it++;
		else
			it = occupants.erase(it);
	}

	if (hasChildren)
	for (int i = 0; i < 4; i++)
		children[i]->pruneDeadReferences();
}
