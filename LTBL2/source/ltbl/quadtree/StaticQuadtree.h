#pragma once

#include "Quadtree.h"

namespace ltbl {
	class StaticQuadtree : public Quadtree
	{
	public:
		StaticQuadtree() {}
		StaticQuadtree(const sf::FloatRect &rootRegion) {
			pRootNode = std::make_unique<QuadtreeNode>(rootRegion, 0, nullptr, this);
		}

		StaticQuadtree(const StaticQuadtree &other) : Quadtree(other) {
			*this = other;
		}

		void operator=(const StaticQuadtree &other) {
			Quadtree::operator=(other);
		}

		void create(const sf::FloatRect &rootRegion) {
			pRootNode = std::make_unique<QuadtreeNode>(rootRegion, 0, nullptr, this);
		}

		// Inherited from Quadtree
		void add(QuadtreeOccupant* oc);

		void clear() {
			pRootNode.reset();
		}

		const sf::FloatRect &getRootRegion() const {
			return pRootNode->getRegion();
		}

		bool created() const {
			return pRootNode != nullptr;
		}
	};
}
