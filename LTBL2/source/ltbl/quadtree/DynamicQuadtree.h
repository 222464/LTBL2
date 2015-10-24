#pragma once

#include <ltbl/quadtree/Quadtree.h>

namespace ltbl {
	class DynamicQuadtree : public Quadtree {
	private:
		void expand();
		void contract();

	public:
		size_t _minOutsideRoot;
		size_t _maxOutsideRoot;

		DynamicQuadtree()
			: _minOutsideRoot(1), _maxOutsideRoot(8)
		{}

		DynamicQuadtree(const sf::FloatRect &rootRegion)
			: _minOutsideRoot(1), _maxOutsideRoot(8)
		{
			_pRootNode = std::make_unique<QuadtreeNode>(rootRegion, 0, nullptr, this);
		}

		DynamicQuadtree(const DynamicQuadtree &other) : Quadtree(other) {
			*this = other;
		}

		void operator=(const DynamicQuadtree &other);

		void create(const sf::FloatRect &rootRegion) {
			_pRootNode = std::make_unique<QuadtreeNode>(rootRegion, 0, nullptr, this);
		}

		// Inherited from Quadtree
		void add(QuadtreeOccupant* oc);

		void clear() {
			_pRootNode.reset();
		}

		// Resizes Quadtree
		void trim();

		bool created() const {
			return _pRootNode != nullptr;
		}

		const sf::FloatRect &getRootRegion() const {
			return _pRootNode->getRegion();
		}
	};
}
