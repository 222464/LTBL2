#pragma once

#include <SFML/Graphics.hpp>

namespace ltbl {
	const float _pi = 3.14159265f;
	const float _radToDeg = 180.0f / _pi;

	sf::Vector2f rectCenter(const sf::FloatRect &rect);
	bool rectContains(const sf::FloatRect &rect, const sf::FloatRect &other);
	bool rectIntersects(const sf::FloatRect &rect, const sf::FloatRect &other);
	sf::Vector2f rectHalfDims(const sf::FloatRect &rect);
	sf::Vector2f rectDims(const sf::FloatRect &rect);
	sf::Vector2f rectLowerBound(const sf::FloatRect &rect);
	sf::Vector2f rectUpperBound(const sf::FloatRect &rect);
	sf::FloatRect rectFromBounds(const sf::Vector2f &lowerBound, const sf::Vector2f &upperBound);
	float vectorMagnitude(const sf::Vector2f &vector);
	float vectorMagnitudeSquared(const sf::Vector2f &vector);
	sf::Vector2f vectorNormalize(const sf::Vector2f &vector);
	float vectorProject(const sf::Vector2f &left, const sf::Vector2f &right);
	sf::FloatRect rectRecenter(const sf::FloatRect &rect, const sf::Vector2f &center);
	float vectorDot(const sf::Vector2f &left, const sf::Vector2f &right);
	sf::FloatRect rectExpand(const sf::FloatRect &rect, const sf::Vector2f &point);
	bool shapeIntersection(const sf::ConvexShape &left, const sf::ConvexShape &right);
	sf::ConvexShape shapeFromRect(const sf::FloatRect &rect);
	sf::ConvexShape shapeFixWinding(const sf::ConvexShape &shape);
	bool rayIntersect(const sf::Vector2f &as, const sf::Vector2f &ad, const sf::Vector2f &bs, const sf::Vector2f &bd, sf::Vector2f &intersection);
}