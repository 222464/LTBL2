#pragma once

#include "../quadtree/QuadtreeOccupant.h"

namespace ltbl {
	class LightShape : public QuadtreeOccupant {
	public:
		bool renderLightOverShape;

		sf::ConvexShape shape;

		LightShape()
			: renderLightOverShape(true)
		{}

		sf::FloatRect getAABB() const {
			return shape.getGlobalBounds();
		}
	};
}