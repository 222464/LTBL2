#pragma once

#include <ltbl/quadtree/QuadtreeNode.h>

#include <memory>

#include <unordered_set>
#include <list>

#include <mutex>
#include <thread>

namespace ltbl {
	// Base class for dynamic and static Quadtree types
	class Quadtree {
	protected:
		std::unordered_set<QuadtreeOccupant*> _outsideRoot;

		std::unique_ptr<QuadtreeNode> _pRootNode;

		// Called whenever something is removed, an action can be defined by derived classes
		// Defaults to doing nothing
		virtual void onRemoval() {}

		void setQuadtree(QuadtreeOccupant* oc);

		void recursiveCopy(QuadtreeNode* pThisNode, QuadtreeNode* pOtherNode, QuadtreeNode* pThisParent);

	public:
		size_t _minNumNodeOccupants;
		size_t _maxNumNodeOccupants;
		size_t _maxLevels;

		float _oversizeMultiplier;

		Quadtree();
		Quadtree(const Quadtree &other) {
			*this = other;
		}

		virtual ~Quadtree() {}

		void operator=(const Quadtree &other);

		virtual void add(QuadtreeOccupant* oc) = 0;

		void pruneDeadReferences();

		void queryRegion(std::vector<QuadtreeOccupant*> &result, const sf::FloatRect &region);
		void queryPoint(std::vector<QuadtreeOccupant*> &result, const sf::Vector2f &p);
		void queryShape(std::vector<QuadtreeOccupant*> &result, const sf::ConvexShape &shape);

		friend class QuadtreeNode;
		friend class SceneObject;
		friend class QuadtreeOccupant;
	};
}
