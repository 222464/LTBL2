#pragma once

#include <ltbl/quadtree/QuadtreeOccupant.h>

#include <memory>
#include <array>
#include <unordered_set>

namespace ltbl {
	class QuadtreeNode : public sf::NonCopyable {
	private:
		QuadtreeNode* _pParent;
		class Quadtree* _pQuadtree;

		bool _hasChildren;

		std::array<std::unique_ptr<QuadtreeNode>, 4> _children;

		std::unordered_set<QuadtreeOccupant*> _occupants;

		sf::FloatRect _region;

		unsigned _level;

		unsigned _numOccupantsBelow;

		void getPossibleOccupantPosition(QuadtreeOccupant* oc, sf::Vector2i &point);

		void addToThisLevel(QuadtreeOccupant* oc);

		// Returns true if occupant was added to children
		bool addToChildren(QuadtreeOccupant* oc);

		void destroyChildren() {
			for (int i = 0; i < 4; i++)
				_children[i].reset();

			_hasChildren = false;
		}

		void getOccupants(std::unordered_set<QuadtreeOccupant*> &occupants);

		void partition();

		void merge();

		void update(QuadtreeOccupant* oc);
		void remove(QuadtreeOccupant* oc);

		void removeForDeletion(std::unordered_set<QuadtreeOccupant*> &occupants);

	public:
		QuadtreeNode()
			: _hasChildren(false), _numOccupantsBelow(0)
		{}

		QuadtreeNode(const sf::FloatRect &region, int level, QuadtreeNode* pParent, class Quadtree* pQuadtree);

		// For use after using default constructor
		void create(const sf::FloatRect &region, int level, QuadtreeNode* pParent, class Quadtree* pQuadtree);

		class Quadtree* getTree() const {
			return _pQuadtree;
		}

		void add(QuadtreeOccupant* oc);

		const sf::FloatRect &getRegion() const {
			return _region;
		}

		void getAllOccupantsBelow(std::vector<QuadtreeOccupant*> &occupants);
		void getAllOccupantsBelow(std::unordered_set<QuadtreeOccupant*> &occupants);

		int getNumOccupantsBelow() const {
			return _numOccupantsBelow;
		}

		void pruneDeadReferences();

		friend class QuadtreeOccupant;
		friend class Quadtree;
		friend class DynamicQuadtree;
	};
}
