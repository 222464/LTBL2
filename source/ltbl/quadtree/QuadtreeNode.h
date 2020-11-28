#pragma once

#include "QuadtreeOccupant.h"

#include <memory>
#include <array>
#include <unordered_set>

namespace ltbl {
	class QuadtreeNode : public sf::NonCopyable {
	private:
		QuadtreeNode* pParent;
		class Quadtree* pQuadtree;

		bool hasChildren;

		std::array<std::unique_ptr<QuadtreeNode>, 4> children;

		std::unordered_set<QuadtreeOccupant*> occupants;

		sf::FloatRect region;

		unsigned level;

		unsigned numOccupantsBelow;

		void getPossibleOccupantPosition(QuadtreeOccupant* oc, sf::Vector2i &point);

		void addToThisLevel(QuadtreeOccupant* oc);

		// Returns true if occupant was added to children
		bool addToChildren(QuadtreeOccupant* oc);

		void destroyChildren() {
			for (int i = 0; i < 4; i++)
				children[i].reset();

			hasChildren = false;
		}

		void getOccupants(std::unordered_set<QuadtreeOccupant*> &occupants);

		void partition();

		void merge();

		void update(QuadtreeOccupant* oc);
		void remove(QuadtreeOccupant* oc);

		void removeForDeletion(std::unordered_set<QuadtreeOccupant*> &occupants);

	public:
		QuadtreeNode()
			: hasChildren(false), numOccupantsBelow(0)
		{}

		QuadtreeNode(const sf::FloatRect &region, int level, QuadtreeNode* pParent, class Quadtree* pQuadtree);

		// For use after using default constructor
		void create(const sf::FloatRect &region, int level, QuadtreeNode* pParent, class Quadtree* pQuadtree);

		class Quadtree* getTree() const {
			return pQuadtree;
		}

		void add(QuadtreeOccupant* oc);

		const sf::FloatRect &getRegion() const {
			return region;
		}

		void getAllOccupantsBelow(std::vector<QuadtreeOccupant*> &occupants);
		void getAllOccupantsBelow(std::unordered_set<QuadtreeOccupant*> &occupants);

		int getNumOccupantsBelow() const {
			return numOccupantsBelow;
		}

		void pruneDeadReferences();

		friend class QuadtreeOccupant;
		friend class Quadtree;
		friend class DynamicQuadtree;
	};
}
