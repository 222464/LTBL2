#pragma once

#include "Quadtree.h"

namespace ltbl {
	class DynamicQuadtree : public Quadtree {
	private:
		void expand();
		void contract();

	public:
		size_t minOutsideRoot;
		size_t maxOutsideRoot;

		DynamicQuadtree()
			: minOutsideRoot(1), maxOutsideRoot(8)
		{}

		DynamicQuadtree(const sf::FloatRect &rootRegion)
			: minOutsideRoot(1), maxOutsideRoot(8)
		{
			pRootNode = std::make_unique<QuadtreeNode>(rootRegion, 0, nullptr, this);
		}

		DynamicQuadtree(const DynamicQuadtree &other) : Quadtree(other) {
			*this = other;
		}

		void operator=(const DynamicQuadtree &other);

		void create(const sf::FloatRect &rootRegion) {
			pRootNode = std::make_unique<QuadtreeNode>(rootRegion, 0, nullptr, this);
		}

		// Inherited from Quadtree
		void add(QuadtreeOccupant* oc);

		void clear() {
			pRootNode.reset();
		}

		// Resizes Quadtree
		void trim();

		bool created() const {
			return pRootNode != nullptr;
		}

		const sf::FloatRect &getRootRegion() const {
			return pRootNode->getRegion();
		}
	};
}
