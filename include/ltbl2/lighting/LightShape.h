#pragma once

#include <ltbl2/quadtree/QuadtreeOccupant.h>

namespace ltbl {
    class LightShape : public QuadtreeOccupant {
    public:
        bool _renderLightOverShape;

        sf::ConvexShape _shape;

        LightShape()
            : _renderLightOverShape(true)
        {}

        sf::FloatRect getAABB() const {
            return _shape.getGlobalBounds();
        }
    };
}