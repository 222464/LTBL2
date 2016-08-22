#include <ltbl/Math.h>

#include <list>

#include <assert.h>

#include <cmath>

using namespace ltbl;

sf::Vector2f ltbl::rectCenter(const sf::FloatRect &rect) {
	return sf::Vector2f(rect.left + rect.width * 0.5f, rect.top + rect.height * 0.5f);
}

bool ltbl::rectIntersects(const sf::FloatRect &rect, const sf::FloatRect &other) {
	if (rect.left + rect.width < other.left)
		return false;
	if (rect.top + rect.height < other.top)
		return false;
	if (rect.left > other.left + other.width)
		return false;
	if (rect.top > other.top + other.height)
		return false;

	return true;
}

bool ltbl::rectContains(const sf::FloatRect &rect, const sf::FloatRect &other) {
	if (other.left < rect.left)
		return false;
	if (other.top < rect.top)
		return false;
	if (other.left + other.width > rect.left + rect.width)
		return false;
	if (other.top + other.height > rect.top + rect.height)
		return false;

	return true;
}

sf::Vector2f ltbl::rectHalfDims(const sf::FloatRect &rect) {
	return sf::Vector2f(rect.width * 0.5f, rect.height * 0.5f);
}

sf::Vector2f ltbl::rectDims(const sf::FloatRect &rect) {
	return sf::Vector2f(rect.width, rect.height);
}

sf::Vector2f ltbl::rectLowerBound(const sf::FloatRect &rect) {
	return sf::Vector2f(rect.left, rect.top);
}

sf::Vector2f ltbl::rectUpperBound(const sf::FloatRect &rect) {
	return sf::Vector2f(rect.left + rect.width, rect.top + rect.height);
}

sf::FloatRect ltbl::rectFromBounds(const sf::Vector2f &lowerBound, const sf::Vector2f &upperBound) {
	return sf::FloatRect(lowerBound.x, lowerBound.y, upperBound.x - lowerBound.x, upperBound.y - lowerBound.y);
}

float ltbl::vectorMagnitude(const sf::Vector2f &vector) {
	return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

float ltbl::vectorMagnitudeSquared(const sf::Vector2f &vector) {
	return vector.x * vector.x + vector.y * vector.y;
}

sf::Vector2f ltbl::vectorNormalize(const sf::Vector2f &vector) {
	float magnitude = vectorMagnitude(vector);

	if (magnitude == 0.0f)
		return sf::Vector2f(1.0f, 0.0f);

	float distInv = 1.0f / magnitude;

	return sf::Vector2f(vector.x * distInv, vector.y * distInv);
}

float ltbl::vectorProject(const sf::Vector2f &left, const sf::Vector2f &right) {
	assert(vectorMagnitudeSquared(right) != 0.0f);

	return vectorDot(left, right) / vectorMagnitudeSquared(right);
}

sf::FloatRect ltbl::rectRecenter(const sf::FloatRect &rect, const sf::Vector2f &center) {
	sf::Vector2f dims = rectDims(rect);

	return sf::FloatRect(center - rectHalfDims(rect), dims);
}

float ltbl::vectorDot(const sf::Vector2f &left, const sf::Vector2f &right) {
	return left.x * right.x + left.y * right.y;
}

sf::FloatRect ltbl::rectExpand(const sf::FloatRect &rect, const sf::Vector2f &point) {
	sf::Vector2f lowerBound = rectLowerBound(rect);
	sf::Vector2f upperBound = rectUpperBound(rect);

	if (point.x < lowerBound.x)
		lowerBound.x = point.x;
	else if (point.x > upperBound.x)
		upperBound.x = point.x;

	if (point.y < lowerBound.y)
		lowerBound.y = point.y;
	else if (point.y > upperBound.y)
		upperBound.y = point.y;

	return rectFromBounds(lowerBound, upperBound);
}

bool ltbl::shapeIntersection(const sf::ConvexShape &left, const sf::ConvexShape &right) {
	std::vector<sf::Vector2f> transformedLeft(left.getPointCount());

	for (unsigned i = 0; i < left.getPointCount(); i++)
		transformedLeft[i] = left.getTransform().transformPoint(left.getPoint(i));

	std::vector<sf::Vector2f> transformedRight(right.getPointCount());

	for (unsigned i = 0; i < right.getPointCount(); i++)
		transformedRight[i] = right.getTransform().transformPoint(right.getPoint(i));

	for (unsigned i = 0; i < left.getPointCount(); i++) {
		sf::Vector2f point = transformedLeft[i];
		sf::Vector2f nextPoint;

		if (i == left.getPointCount() - 1)
			nextPoint = transformedLeft[0];
		else
			nextPoint = transformedLeft[i + 1];

		sf::Vector2f edge = nextPoint - point;

		// Project points from other shape onto perpendicular
		sf::Vector2f edgePerpendicular = sf::Vector2f(edge.y, -edge.x);
		
		float pointProj = vectorProject(point, edgePerpendicular);

		float minRightProj = vectorProject(transformedRight[0], edgePerpendicular);

		for (unsigned j = 1; j < right.getPointCount(); j++) {
			float proj = vectorProject(transformedRight[j], edgePerpendicular);

			minRightProj = std::min(minRightProj, proj);
		}

		if (minRightProj > pointProj)
			return false;
	}

	for (unsigned i = 0; i < right.getPointCount(); i++) {
		sf::Vector2f point = transformedRight[i];
		sf::Vector2f nextPoint;

		if (i == right.getPointCount() - 1)
			nextPoint = transformedRight[0];
		else
			nextPoint = transformedRight[i + 1];

		sf::Vector2f edge = nextPoint - point;

		// Project points from other shape onto perpendicular
		sf::Vector2f edgePerpendicular = sf::Vector2f(edge.y, -edge.x);

		float pointProj = vectorProject(point, edgePerpendicular);

		float minRightProj = vectorProject(transformedLeft[0], edgePerpendicular);

		for (unsigned j = 1; j < left.getPointCount(); j++) {
			float proj = vectorProject(transformedLeft[j], edgePerpendicular);

			minRightProj = std::min(minRightProj, proj);
		}

		if (minRightProj > pointProj)
			return false;
	}

	return true;
}

sf::ConvexShape ltbl::shapeFromRect(const sf::FloatRect &rect) {
	sf::ConvexShape shape(4);

	sf::Vector2f halfDims = rectHalfDims(rect);

	shape.setPoint(0, sf::Vector2f(-halfDims.x, -halfDims.y));
	shape.setPoint(1, sf::Vector2f(halfDims.x, -halfDims.y));
	shape.setPoint(2, sf::Vector2f(halfDims.x, halfDims.y));
	shape.setPoint(3, sf::Vector2f(-halfDims.x, halfDims.y));

	shape.setPosition(rectCenter(rect));

	return shape;
}

sf::ConvexShape ltbl::shapeFixWinding(const sf::ConvexShape &shape) {
	sf::Vector2f center = sf::Vector2f(0.0f, 0.0f);
	std::list<sf::Vector2f> points;

	for (unsigned i = 0; i < shape.getPointCount(); i++) {
		points.push_back(shape.getPoint(i));
		center += shape.getPoint(i);
	}

	center /= static_cast<float>(shape.getPointCount());

	// Fix winding
	sf::Vector2f lastPoint = points.front();
	points.pop_front();

	std::vector<sf::Vector2f> fixedPoints;

	fixedPoints.push_back(lastPoint);

	while (fixedPoints.size() < shape.getPointCount()) {
		sf::Vector2f centerToLastPoint = lastPoint - center;
		sf::Vector2f lastPointDirection = ltbl::vectorNormalize(sf::Vector2f(-centerToLastPoint.y, centerToLastPoint.x));

		float maxD = -999999.0f;

		std::list<sf::Vector2f>::iterator nextPointIt;

		// Get next point
		for (std::list<sf::Vector2f>::iterator it = points.begin(); it != points.end(); it++) {
			sf::Vector2f toPointNormalized = ltbl::vectorNormalize(*it - lastPoint);

			float d = ltbl::vectorDot(toPointNormalized, lastPointDirection);

			if (d > maxD) {
				maxD = d;
				nextPointIt = it;
			}
		}

		fixedPoints.push_back(*nextPointIt);

		points.erase(nextPointIt);
	}

	sf::ConvexShape fixedShape(shape.getPointCount());

	for (unsigned i = 0; i < shape.getPointCount(); i++)
		fixedShape.setPoint(i, fixedPoints[i]);

	return fixedShape;
}

bool ltbl::rayIntersect(const sf::Vector2f &as, const sf::Vector2f &ad, const sf::Vector2f &bs, const sf::Vector2f &bd, sf::Vector2f &intersection) {
	float dx = bs.x - as.x;
	float dy = bs.y - as.y;
	float det = bd.x * ad.y - bd.y * ad.x;

	if (det == 0.0f)
		return false;

	float u = (dy * bd.x - dx * bd.y) / det;

	if (u < 0.0f)
		return false;

	float v = (dy * ad.x - dx * ad.y) / det;

	if (v < 0.0f)
		return false;

	intersection = as + ad * u;

	return true;
}
