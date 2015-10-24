#pragma once

#include <ltbl/quadtree/Quadtree.h>

namespace ltbl {
	class StaticQuadtree : public Quadtree
	{
	public:
		StaticQuadtree() {}
		StaticQuadtree(const sf::FloatRect &rootRegion) {
			_pRootNode.reset(new QuadtreeNode(rootRegion, 0, nullptr, this));
		}

		StaticQuadtree(const StaticQuadtree &other) : Quadtree(other) {
			*this = other;
		}

		void operator=(const StaticQuadtree &other) {
			Quadtree::operator=(other);
		}

		void create(const sf::FloatRect &rootRegion) {
			_pRootNode.reset(new QuadtreeNode(rootRegion, 0, nullptr, this));
		}

		// Inherited from Quadtree
		void add(QuadtreeOccupant* oc);

		void clear() {
			_pRootNode.reset();
		}

		const sf::FloatRect &getRootRegion() const {
			return _pRootNode->getRegion();
		}

		bool created() const {
			return _pRootNode != nullptr;
		}
	};
}
