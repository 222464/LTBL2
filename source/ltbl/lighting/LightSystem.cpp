#include "LightSystem.h"

#include <cmath>

#include <assert.h>

#include <iostream>

using namespace ltbl;

void LightSystem::getPenumbrasPoint(std::vector<Penumbra> &penumbras, std::vector<int> &innerBoundaryIndices, std::vector<sf::Vector2f> &innerBoundaryVectors, std::vector<int> &outerBoundaryIndices, std::vector<sf::Vector2f> &outerBoundaryVectors, const sf::ConvexShape &shape, const sf::Vector2f &sourceCenter, float sourceRadius) {
	const int numPoints = shape.getPointCount();

	std::vector<bool> bothEdgesBoundaryWindings;
	bothEdgesBoundaryWindings.reserve(2);

	std::vector<bool> oneEdgeBoundaryWindings;
	oneEdgeBoundaryWindings.reserve(2);

	// Calculate front and back facing sides
	std::vector<bool> facingFrontBothEdges;
	facingFrontBothEdges.reserve(numPoints);

	std::vector<bool> facingFrontOneEdge;
	facingFrontOneEdge.reserve(numPoints);

	for (int i = 0; i < numPoints; i++) {
		sf::Vector2f point = shape.getTransform().transformPoint(shape.getPoint(i));

		sf::Vector2f nextPoint;

		if (i < numPoints - 1)
			nextPoint = shape.getTransform().transformPoint(shape.getPoint(i + 1));
		else
			nextPoint = shape.getTransform().transformPoint(shape.getPoint(0));

		sf::Vector2f firstEdgeRay;
		sf::Vector2f secondEdgeRay;
		sf::Vector2f firstNextEdgeRay;
		sf::Vector2f secondNextEdgeRay;

		{
			sf::Vector2f sourceToPoint = point - sourceCenter;

			sf::Vector2f perpendicularOffset(-sourceToPoint.y, sourceToPoint.x);

			perpendicularOffset = vectorNormalize(perpendicularOffset);
			perpendicularOffset *= sourceRadius;

			firstEdgeRay = point - (sourceCenter - perpendicularOffset);
			secondEdgeRay = point - (sourceCenter + perpendicularOffset);
		}

		{
			sf::Vector2f sourceToPoint = nextPoint - sourceCenter;

			sf::Vector2f perpendicularOffset(-sourceToPoint.y, sourceToPoint.x);

			perpendicularOffset = vectorNormalize(perpendicularOffset);
			perpendicularOffset *= sourceRadius;

			firstNextEdgeRay = nextPoint - (sourceCenter - perpendicularOffset);
			secondNextEdgeRay = nextPoint - (sourceCenter + perpendicularOffset);
		}

		sf::Vector2f pointToNextPoint = nextPoint - point;

		sf::Vector2f normal = vectorNormalize(sf::Vector2f(-pointToNextPoint.y, pointToNextPoint.x));

		// Front facing, mark it
		facingFrontBothEdges.push_back((vectorDot(firstEdgeRay, normal) > 0.0f && vectorDot(secondEdgeRay, normal) > 0.0f) || (vectorDot(firstNextEdgeRay, normal) > 0.0f && vectorDot(secondNextEdgeRay, normal) > 0.0f));
		facingFrontOneEdge.push_back((vectorDot(firstEdgeRay, normal) > 0.0f || vectorDot(secondEdgeRay, normal) > 0.0f) || vectorDot(firstNextEdgeRay, normal) > 0.0f || vectorDot(secondNextEdgeRay, normal) > 0.0f);
	}

	// Go through front/back facing list. Where the facing direction switches, there is a boundary
	for (int i = 1; i < numPoints; i++)
		if (facingFrontBothEdges[i] != facingFrontBothEdges[i - 1]) {
			innerBoundaryIndices.push_back(i);
			bothEdgesBoundaryWindings.push_back(facingFrontBothEdges[i]);
		}

	// Check looping indices separately
	if (facingFrontBothEdges[0] != facingFrontBothEdges[numPoints - 1]) {
		innerBoundaryIndices.push_back(0);
		bothEdgesBoundaryWindings.push_back(facingFrontBothEdges[0]);
	}

	// Go through front/back facing list. Where the facing direction switches, there is a boundary
	for (int i = 1; i < numPoints; i++)
		if (facingFrontOneEdge[i] != facingFrontOneEdge[i - 1]) {
			outerBoundaryIndices.push_back(i);
			oneEdgeBoundaryWindings.push_back(facingFrontOneEdge[i]);
		}

	// Check looping indices separately
	if (facingFrontOneEdge[0] != facingFrontOneEdge[numPoints - 1]) {
		outerBoundaryIndices.push_back(0);
		oneEdgeBoundaryWindings.push_back(facingFrontOneEdge[0]);
	}

	// Compute outer boundary vectors
	for (unsigned bi = 0; bi < outerBoundaryIndices.size(); bi++) {
		int penumbraIndex = outerBoundaryIndices[bi];
		bool winding = oneEdgeBoundaryWindings[bi];

		sf::Vector2f point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

		sf::Vector2f sourceToPoint = point - sourceCenter;

		sf::Vector2f perpendicularOffset(-sourceToPoint.y, sourceToPoint.x);

		perpendicularOffset = vectorNormalize(perpendicularOffset);
		perpendicularOffset *= sourceRadius;

		sf::Vector2f firstEdgeRay = point - (sourceCenter + perpendicularOffset);
		sf::Vector2f secondEdgeRay = point - (sourceCenter - perpendicularOffset);

		// Add boundary vector
		outerBoundaryVectors.push_back(winding ? firstEdgeRay : secondEdgeRay);
	}

	for (unsigned bi = 0; bi < innerBoundaryIndices.size(); bi++) {
		int penumbraIndex = innerBoundaryIndices[bi];
		bool winding = bothEdgesBoundaryWindings[bi];

		sf::Vector2f point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

		sf::Vector2f sourceToPoint = point - sourceCenter;

		sf::Vector2f perpendicularOffset(-sourceToPoint.y, sourceToPoint.x);

		perpendicularOffset = vectorNormalize(perpendicularOffset);
		perpendicularOffset *= sourceRadius;

		sf::Vector2f firstEdgeRay = point - (sourceCenter + perpendicularOffset);
		sf::Vector2f secondEdgeRay = point - (sourceCenter - perpendicularOffset);

		// Add boundary vector
		innerBoundaryVectors.push_back(winding ? secondEdgeRay : firstEdgeRay);
		sf::Vector2f outerBoundaryVector = winding ? firstEdgeRay : secondEdgeRay;

		if (innerBoundaryIndices.size() == 1)
			innerBoundaryVectors.push_back(outerBoundaryVector);

		// Add penumbras
		bool hasPrevPenumbra = false;

		sf::Vector2f prevPenumbraLightEdgeVector;

		float prevBrightness = 1.0f;

		int counter = 0;

		while (penumbraIndex != -1) {
			sf::Vector2f nextPoint;
			int nextPointIndex;

			if (penumbraIndex < numPoints - 1) {
				nextPointIndex = penumbraIndex + 1;
				nextPoint = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex + 1));
			}
			else {
				nextPointIndex = 0;
				nextPoint = shape.getTransform().transformPoint(shape.getPoint(0));
			}

			sf::Vector2f pointToNextPoint = nextPoint - point;

			sf::Vector2f prevPoint;
			int prevPointIndex;

			if (penumbraIndex > 0) {
				prevPointIndex = penumbraIndex - 1;
				prevPoint = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex - 1));
			}
			else {
				prevPointIndex = numPoints - 1;
				prevPoint = shape.getTransform().transformPoint(shape.getPoint(numPoints - 1));
			}

			sf::Vector2f pointToPrevPoint = prevPoint - point;

			LightSystem::Penumbra penumbra;

			penumbra.source = point;

			if (!winding) {
				if (hasPrevPenumbra)
					penumbra.lightEdge = prevPenumbraLightEdgeVector;
				else
					penumbra.lightEdge = innerBoundaryVectors.back();

				penumbra.darkEdge = outerBoundaryVector;

				penumbra.lightBrightness = prevBrightness;

				// Next point, check for intersection
				float intersectionAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(pointToNextPoint)));
				float penumbraAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(penumbra.darkEdge)));

				if (intersectionAngle < penumbraAngle) {
					prevBrightness = penumbra.darkBrightness = intersectionAngle / penumbraAngle;

					assert(prevBrightness >= 0.0f && prevBrightness <= 1.0f);

					penumbra.darkEdge = pointToNextPoint;

					penumbraIndex = nextPointIndex;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = true;

					prevPenumbraLightEdgeVector = penumbra.darkEdge;

					point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

					sourceToPoint = point - sourceCenter;

					perpendicularOffset = sf::Vector2f(-sourceToPoint.y, sourceToPoint.x);

					perpendicularOffset = vectorNormalize(perpendicularOffset);
					perpendicularOffset *= sourceRadius;

					firstEdgeRay = point - (sourceCenter + perpendicularOffset);
					secondEdgeRay = point - (sourceCenter - perpendicularOffset);

					outerBoundaryVector = secondEdgeRay;

					if (!outerBoundaryVectors.empty()) {
						outerBoundaryVectors[0] = penumbra.darkEdge;
						outerBoundaryIndices[0] = penumbraIndex;
					}
				}
				else {
					penumbra.darkBrightness = 0.0f;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = false;

					if (!outerBoundaryVectors.empty()) {
						outerBoundaryVectors[0] = penumbra.darkEdge;
						outerBoundaryIndices[0] = penumbraIndex;
					}

					penumbraIndex = -1;
				}
			}
			else {
				if (hasPrevPenumbra)
					penumbra.lightEdge = prevPenumbraLightEdgeVector;
				else
					penumbra.lightEdge = innerBoundaryVectors.back();

				penumbra.darkEdge = outerBoundaryVector;

				penumbra.lightBrightness = prevBrightness;

				// Next point, check for intersection
				float intersectionAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(pointToPrevPoint)));
				float penumbraAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(penumbra.darkEdge)));

				if (intersectionAngle < penumbraAngle) {
					prevBrightness = penumbra.darkBrightness = intersectionAngle / penumbraAngle;

					assert(prevBrightness >= 0.0f && prevBrightness <= 1.0f);

					penumbra.darkEdge = pointToPrevPoint;

					penumbraIndex = prevPointIndex;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = true;

					prevPenumbraLightEdgeVector = penumbra.darkEdge;

					point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

					sourceToPoint = point - sourceCenter;

					perpendicularOffset = sf::Vector2f(-sourceToPoint.y, sourceToPoint.x);

					perpendicularOffset = vectorNormalize(perpendicularOffset);
					perpendicularOffset *= sourceRadius;

					firstEdgeRay = point - (sourceCenter + perpendicularOffset);
					secondEdgeRay = point - (sourceCenter - perpendicularOffset);

					outerBoundaryVector = firstEdgeRay;

					if (!outerBoundaryVectors.empty()) {
						outerBoundaryVectors[1] = penumbra.darkEdge;
						outerBoundaryIndices[1] = penumbraIndex;
					}
				}
				else {
					penumbra.darkBrightness = 0.0f;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = false;

					if (!outerBoundaryVectors.empty()) {
						outerBoundaryVectors[1] = penumbra.darkEdge;
						outerBoundaryIndices[1] = penumbraIndex;
					}

					penumbraIndex = -1;
				}
			}

			penumbras.push_back(penumbra);

			counter++;
		}
	}
}

void LightSystem::getPenumbrasDirection(std::vector<Penumbra> &penumbras, std::vector<int> &innerBoundaryIndices, std::vector<sf::Vector2f> &innerBoundaryVectors, std::vector<int> &outerBoundaryIndices, std::vector<sf::Vector2f> &outerBoundaryVectors, const sf::ConvexShape &shape, const sf::Vector2f &sourceDirection, float sourceRadius, float sourceDistance) {
	const int numPoints = shape.getPointCount();

	innerBoundaryIndices.reserve(2);
	innerBoundaryVectors.reserve(2);
	penumbras.reserve(2);

	std::vector<bool> bothEdgesBoundaryWindings;
	bothEdgesBoundaryWindings.reserve(2);

	// Calculate front and back facing sides
	std::vector<bool> facingFrontBothEdges;
	facingFrontBothEdges.reserve(numPoints);

	std::vector<bool> facingFrontOneEdge;
	facingFrontOneEdge.reserve(numPoints);

	for (int i = 0; i < numPoints; i++) {
		sf::Vector2f point = shape.getTransform().transformPoint(shape.getPoint(i));

		sf::Vector2f nextPoint;

		if (i < numPoints - 1)
			nextPoint = shape.getTransform().transformPoint(shape.getPoint(i + 1));
		else
			nextPoint = shape.getTransform().transformPoint(shape.getPoint(0));

		sf::Vector2f firstEdgeRay;
		sf::Vector2f secondEdgeRay;
		sf::Vector2f firstNextEdgeRay;
		sf::Vector2f secondNextEdgeRay;

		sf::Vector2f perpendicularOffset(-sourceDirection.y, sourceDirection.x);

		perpendicularOffset = vectorNormalize(perpendicularOffset);
		perpendicularOffset *= sourceRadius;

		firstEdgeRay = point - (point - sourceDirection * sourceDistance - perpendicularOffset);
		secondEdgeRay = point - (point - sourceDirection * sourceDistance + perpendicularOffset);

		firstNextEdgeRay = nextPoint - (point - sourceDirection * sourceDistance - perpendicularOffset);
		secondNextEdgeRay = nextPoint - (point - sourceDirection * sourceDistance + perpendicularOffset);

		sf::Vector2f pointToNextPoint = nextPoint - point;

		sf::Vector2f normal = vectorNormalize(sf::Vector2f(-pointToNextPoint.y, pointToNextPoint.x));

		// Front facing, mark it
		facingFrontBothEdges.push_back((vectorDot(firstEdgeRay, normal) > 0.0f && vectorDot(secondEdgeRay, normal) > 0.0f) || (vectorDot(firstNextEdgeRay, normal) > 0.0f && vectorDot(secondNextEdgeRay, normal) > 0.0f));
		facingFrontOneEdge.push_back((vectorDot(firstEdgeRay, normal) > 0.0f || vectorDot(secondEdgeRay, normal) > 0.0f) || (vectorDot(firstNextEdgeRay, normal) > 0.0f || vectorDot(secondNextEdgeRay, normal) > 0.0f));
	}

	// Go through front/back facing list. Where the facing direction switches, there is a boundary
	for (int i = 1; i < numPoints; i++)
		if (facingFrontBothEdges[i] != facingFrontBothEdges[i - 1]) {
			innerBoundaryIndices.push_back(i);
			bothEdgesBoundaryWindings.push_back(facingFrontBothEdges[i]);
		}

	// Check looping indices separately
	if (facingFrontBothEdges[0] != facingFrontBothEdges[numPoints - 1]) {
		innerBoundaryIndices.push_back(0);
		bothEdgesBoundaryWindings.push_back(facingFrontBothEdges[0]);
	}

	// Go through front/back facing list. Where the facing direction switches, there is a boundary
	for (int i = 1; i < numPoints; i++)
		if (facingFrontOneEdge[i] != facingFrontOneEdge[i - 1])
			outerBoundaryIndices.push_back(i);

	// Check looping indices separately
	if (facingFrontOneEdge[0] != facingFrontOneEdge[numPoints - 1])
		outerBoundaryIndices.push_back(0);

	for (unsigned bi = 0; bi < innerBoundaryIndices.size(); bi++) {
		int penumbraIndex = innerBoundaryIndices[bi];
		bool winding = bothEdgesBoundaryWindings[bi];

		sf::Vector2f point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

		sf::Vector2f perpendicularOffset(-sourceDirection.y, sourceDirection.x);

		perpendicularOffset = vectorNormalize(perpendicularOffset);
		perpendicularOffset *= sourceRadius;

		sf::Vector2f firstEdgeRay = point - (point - sourceDirection * sourceDistance + perpendicularOffset);
		sf::Vector2f secondEdgeRay = point - (point - sourceDirection * sourceDistance - perpendicularOffset);

		// Add boundary vector
		innerBoundaryVectors.push_back(winding ? secondEdgeRay : firstEdgeRay);
		sf::Vector2f outerBoundaryVector = winding ? firstEdgeRay : secondEdgeRay;

		outerBoundaryVectors.push_back(outerBoundaryVector);

		// Add penumbras
		bool hasPrevPenumbra = false;

		sf::Vector2f prevPenumbraLightEdgeVector;

		float prevBrightness = 1.0f;

		int counter = 0;

		while (penumbraIndex != -1) {
			sf::Vector2f nextPoint;
			int nextPointIndex;

			if (penumbraIndex < numPoints - 1) {
				nextPointIndex = penumbraIndex + 1;
				nextPoint = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex + 1));
			}
			else {
				nextPointIndex = 0;
				nextPoint = shape.getTransform().transformPoint(shape.getPoint(0));
			}

			sf::Vector2f pointToNextPoint = nextPoint - point;

			sf::Vector2f prevPoint;
			int prevPointIndex;

			if (penumbraIndex > 0) {
				prevPointIndex = penumbraIndex - 1;
				prevPoint = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex - 1));
			}
			else {
				prevPointIndex = numPoints - 1;
				prevPoint = shape.getTransform().transformPoint(shape.getPoint(numPoints - 1));
			}

			sf::Vector2f pointToPrevPoint = prevPoint - point;

			LightSystem::Penumbra penumbra;

			penumbra.source = point;

			if (!winding) {
				if (hasPrevPenumbra)
					penumbra.lightEdge = prevPenumbraLightEdgeVector;
				else
					penumbra.lightEdge = innerBoundaryVectors.back();

				penumbra.darkEdge = outerBoundaryVector;

				penumbra.lightBrightness = prevBrightness;

				// Next point, check for intersection
				float intersectionAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(pointToNextPoint)));
				float penumbraAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(penumbra.darkEdge)));

				if (intersectionAngle < penumbraAngle) {
					prevBrightness = penumbra.darkBrightness = intersectionAngle / penumbraAngle;

					assert(prevBrightness >= 0.0f && prevBrightness <= 1.0f);

					penumbra.darkEdge = pointToNextPoint;

					penumbraIndex = nextPointIndex;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = true;

					prevPenumbraLightEdgeVector = penumbra.darkEdge;

					point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

					perpendicularOffset = sf::Vector2f(-sourceDirection.y, sourceDirection.x);

					perpendicularOffset = vectorNormalize(perpendicularOffset);
					perpendicularOffset *= sourceRadius;

					firstEdgeRay = point - (point - sourceDirection * sourceDistance + perpendicularOffset);
					secondEdgeRay = point - (point - sourceDirection * sourceDistance - perpendicularOffset);

					outerBoundaryVector = secondEdgeRay;
				}
				else {
					penumbra.darkBrightness = 0.0f;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = false;

					penumbraIndex = -1;
				}
			}
			else {
				if (hasPrevPenumbra)
					penumbra.lightEdge = prevPenumbraLightEdgeVector;
				else
					penumbra.lightEdge = innerBoundaryVectors.back();

				penumbra.darkEdge = outerBoundaryVector;

				penumbra.lightBrightness = prevBrightness;

				// Next point, check for intersection
				float intersectionAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(pointToPrevPoint)));
				float penumbraAngle = std::acos(vectorDot(vectorNormalize(penumbra.lightEdge), vectorNormalize(penumbra.darkEdge)));

				if (intersectionAngle < penumbraAngle) {
					prevBrightness = penumbra.darkBrightness = intersectionAngle / penumbraAngle;

					assert(prevBrightness >= 0.0f && prevBrightness <= 1.0f);

					penumbra.darkEdge = pointToPrevPoint;

					penumbraIndex = prevPointIndex;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = true;

					prevPenumbraLightEdgeVector = penumbra.darkEdge;

					point = shape.getTransform().transformPoint(shape.getPoint(penumbraIndex));

					perpendicularOffset = sf::Vector2f(-sourceDirection.y, sourceDirection.x);

					perpendicularOffset = vectorNormalize(perpendicularOffset);
					perpendicularOffset *= sourceRadius;

					firstEdgeRay = point - (point - sourceDirection * sourceDistance + perpendicularOffset);
					secondEdgeRay = point - (point - sourceDirection * sourceDistance - perpendicularOffset);

					outerBoundaryVector = firstEdgeRay;
				}
				else {
					penumbra.darkBrightness = 0.0f;

					if (hasPrevPenumbra) {
						std::swap(penumbra.darkBrightness, penumbras.back().darkBrightness);
						std::swap(penumbra.lightBrightness, penumbras.back().lightBrightness);
					}

					hasPrevPenumbra = false;

					penumbraIndex = -1;
				}
			}

			penumbras.push_back(penumbra);

			counter++;
		}
	}
}
void LightSystem::clear(sf::RenderTarget &rt, const sf::Color &color) {
	sf::RectangleShape shape;
	shape.setSize(sf::Vector2f(rt.getSize().x, rt.getSize().y));
	shape.setFillColor(color);
	sf::View v = rt.getView();
	rt.setView(rt.getDefaultView());
	rt.draw(shape);
	rt.setView(v);
}

void LightSystem::create(const sf::FloatRect &rootRegion, const sf::Vector2u &imageSize, const sf::Texture &penumbraTexture, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader) {
	shapeQuadtree.create(rootRegion);
	lightPointEmissionQuadtree.create(rootRegion);

	lightTempTexture.create(imageSize.x, imageSize.y);
	emissionTempTexture.create(imageSize.x, imageSize.y);
	antumbraTempTexture.create(imageSize.x, imageSize.y);
	compositionTexture.create(imageSize.x, imageSize.y);

	sf::Vector2f targetSizeInv = sf::Vector2f(1.0f / imageSize.x, 1.0f / imageSize.y);

	unshadowShader.setUniform("penumbraTexture", penumbraTexture);

	lightOverShapeShader.setUniform("emissionTexture", emissionTempTexture.getTexture());
	lightOverShapeShader.setUniform("targetSizeInv", targetSizeInv);
}

void LightSystem::render(const sf::View &view, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader) {
	clear(compositionTexture, ambientColor);
	compositionTexture.setView(compositionTexture.getDefaultView());

	// Get bounding rectangle of view
	sf::FloatRect viewBounds = sf::FloatRect(view.getCenter().x, view.getCenter().y, 0.0f, 0.0f);

	lightTempTexture.setView(view);

	viewBounds = rectExpand(viewBounds, lightTempTexture.mapPixelToCoords(sf::Vector2i(0, 0)));
	viewBounds = rectExpand(viewBounds, lightTempTexture.mapPixelToCoords(sf::Vector2i(lightTempTexture.getSize().x, 0)));
	viewBounds = rectExpand(viewBounds, lightTempTexture.mapPixelToCoords(sf::Vector2i(lightTempTexture.getSize().x, lightTempTexture.getSize().y)));
	viewBounds = rectExpand(viewBounds, lightTempTexture.mapPixelToCoords(sf::Vector2i(0, lightTempTexture.getSize().y)));

	std::vector<QuadtreeOccupant*> viewPointEmissionLights;

	lightPointEmissionQuadtree.queryRegion(viewPointEmissionLights, viewBounds);

	for (unsigned l = 0; l < viewPointEmissionLights.size(); l++) {
		LightPointEmission* pPointEmissionLight = static_cast<LightPointEmission*>(viewPointEmissionLights[l]);

		// Query shapes this light is affected by
		std::vector<QuadtreeOccupant*> lightShapes;

		shapeQuadtree.queryRegion(lightShapes, pPointEmissionLight->getAABB());

		pPointEmissionLight->render(view, lightTempTexture, emissionTempTexture, antumbraTempTexture, lightShapes, unshadowShader, lightOverShapeShader);

		sf::Sprite sprite;

		sprite.setTexture(lightTempTexture.getTexture());

		sf::RenderStates compoRenderStates;
		compoRenderStates.blendMode = sf::BlendAdd;

		compositionTexture.draw(sprite, compoRenderStates);
	}
	
	for (std::unordered_set<std::shared_ptr<LightDirectionEmission>>::iterator it = directionEmissionLights.begin(); it != directionEmissionLights.end(); it++) {
		LightDirectionEmission* pDirectionEmissionLight = static_cast<LightDirectionEmission*>(it->get());

		sf::FloatRect centeredViewBounds = rectRecenter(viewBounds, sf::Vector2f(0.0f, 0.0f));

		float maxDim = std::max(centeredViewBounds.width, centeredViewBounds.height);

		sf::FloatRect extendedViewBounds = rectFromBounds(sf::Vector2f(-maxDim, -maxDim) * directionEmissionRadiusMultiplier,
			sf::Vector2f(maxDim, maxDim) * directionEmissionRadiusMultiplier + sf::Vector2f(directionEmissionRange, 0.0f));

		float shadowExtension = vectorMagnitude(rectLowerBound(centeredViewBounds)) * directionEmissionRadiusMultiplier * 2.0f;

		sf::ConvexShape directionShape = shapeFromRect(extendedViewBounds);

		directionShape.setPosition(view.getCenter());

		sf::Vector2f normalizedCastDirection = vectorNormalize(pDirectionEmissionLight->castDirection);

		directionShape.setRotation(radToDeg * std::atan2(normalizedCastDirection.y, normalizedCastDirection.x));

		std::vector<QuadtreeOccupant*> viewLightShapes;

		shapeQuadtree.queryShape(viewLightShapes, directionShape);

		pDirectionEmissionLight->render(view, lightTempTexture, antumbraTempTexture, viewLightShapes, unshadowShader, shadowExtension);

		sf::Sprite sprite;

		sprite.setTexture(lightTempTexture.getTexture());

		sf::RenderStates compoRenderStates;
		compoRenderStates.blendMode = sf::BlendAdd;

		compositionTexture.draw(sprite, compoRenderStates);
	}

	compositionTexture.display();
}

void LightSystem::addShape(const std::shared_ptr<LightShape> &lightShape) {
	shapeQuadtree.add(lightShape.get());

	lightShapes.insert(lightShape);
}

void LightSystem::removeShape(const std::shared_ptr<LightShape> &lightShape) {
	std::unordered_set<std::shared_ptr<LightShape>>::iterator it = lightShapes.find(lightShape);

	if (it != lightShapes.end()) {
		(*it)->quadtreeRemove();

		lightShapes.erase(it);
	}
}

void LightSystem::addLight(const std::shared_ptr<LightPointEmission> &pointEmissionLight) {
	lightPointEmissionQuadtree.add(pointEmissionLight.get());

	pointEmissionLights.insert(pointEmissionLight);
}

void LightSystem::addLight(const std::shared_ptr<LightDirectionEmission> &directionEmissionLight) {
	directionEmissionLights.insert(directionEmissionLight);
}

void LightSystem::removeLight(const std::shared_ptr<LightPointEmission> &pointEmissionLight) {
	std::unordered_set<std::shared_ptr<LightPointEmission>>::iterator it = pointEmissionLights.find(pointEmissionLight);

	if (it != pointEmissionLights.end()) {
		(*it)->quadtreeRemove();

		pointEmissionLights.erase(it);
	}
}

void LightSystem::removeLight(const std::shared_ptr<LightDirectionEmission> &directionEmissionLight) {
	std::unordered_set<std::shared_ptr<LightDirectionEmission>>::iterator it = directionEmissionLights.find(directionEmissionLight);

	if (it != directionEmissionLights.end())
		directionEmissionLights.erase(it);
}
