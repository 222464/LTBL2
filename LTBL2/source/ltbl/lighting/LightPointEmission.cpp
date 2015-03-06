#include <ltbl/lighting/LightPointEmission.h>

#include <ltbl/lighting/LightShape.h>

#include <ltbl/lighting/LightSystem.h>

#include <assert.h>

using namespace ltbl;

void LightPointEmission::render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &emissionTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader) {
	LightSystem::clear(emissionTempTexture, sf::Color::Black);

	emissionTempTexture.setView(view);

	emissionTempTexture.draw(_emissionSprite);

	emissionTempTexture.display();
	
	LightSystem::clear(lightTempTexture, sf::Color::Black);

	lightTempTexture.setView(view);

	lightTempTexture.draw(_emissionSprite);

	sf::Vector2f castCenter = _emissionSprite.getTransform().transformPoint(_localCastCenter);
	
	float shadowExtension = _shadowOverExtendMultiplier * (getAABB().width + getAABB().height);

	struct OuterEdges {
		std::vector<int> _outerBoundaryIndices;
		std::vector<sf::Vector2f> _outerBoundaryVectors;
	};

	std::vector<OuterEdges> outerEdges(shapes.size());

	sf::VertexArray vertexArray;

	vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

	vertexArray.resize(3);

	vertexArray[0].texCoords = sf::Vector2f(0.0f, 1.0f);
	vertexArray[1].texCoords = sf::Vector2f(1.0f, 0.0f);
	vertexArray[2].texCoords = sf::Vector2f(0.0f, 0.0f);

	sf::RenderStates penumbraRenderStates;
	penumbraRenderStates.blendMode = sf::BlendAdd;
	penumbraRenderStates.shader = &unshadowShader;

	// Mask off light shape (over-masking - mask too much, reveal penumbra/antumbra afterwards)
	for (int i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		// Get boundaries
		std::vector<int> innerBoundaryIndices;
		std::vector<sf::Vector2f> innerBoundaryVectors;

		std::vector<LightSystem::Penumbra> penumbras;

		LightSystem::getPenumbrasPoint(penumbras, innerBoundaryIndices, innerBoundaryVectors, outerEdges[i]._outerBoundaryIndices, outerEdges[i]._outerBoundaryVectors, pLightShape->_shape, castCenter, _sourceRadius);

		// Render shape
		if (!pLightShape->_renderLightOverShape) {
			pLightShape->_shape.setFillColor(sf::Color::Black);

			lightTempTexture.draw(pLightShape->_shape);
		}

		if (innerBoundaryIndices.size() == 2) {
			sf::Vector2f as = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0]));
			sf::Vector2f bs = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[1]));
			sf::Vector2f ad = innerBoundaryVectors[0];
			sf::Vector2f bd = innerBoundaryVectors[1];

			sf::RenderStates maskRenderStates;
			maskRenderStates.blendMode = sf::BlendNone;

			sf::ConvexShape maskShape;

			maskShape.setPointCount(4);

			maskShape.setPoint(0, as);
			maskShape.setPoint(1, bs);
			maskShape.setPoint(2, bs + vectorNormalize(bd) * shadowExtension);
			maskShape.setPoint(3, as + vectorNormalize(ad) * shadowExtension);

			maskShape.setFillColor(sf::Color::Black);

			lightTempTexture.draw(maskShape, maskRenderStates);
		}
		else if (innerBoundaryIndices.size() == 1) {
			sf::Vector2f as = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0]));
			sf::Vector2f ad = innerBoundaryVectors[0];
			sf::Vector2f bd = innerBoundaryVectors[1];

			sf::RenderStates maskRenderStates;
			maskRenderStates.blendMode = sf::BlendNone;

			sf::ConvexShape maskShape;

			maskShape.setPointCount(3);

			maskShape.setPoint(0, as);
			maskShape.setPoint(1, as + vectorNormalize(bd) * shadowExtension);
			maskShape.setPoint(2, as + vectorNormalize(ad) * shadowExtension);

			maskShape.setFillColor(sf::Color::Black);

			lightTempTexture.draw(maskShape, maskRenderStates);
		}

		// Mask with penumbras
		for (int j = 0; j < penumbras.size(); j++) {
			unshadowShader.setParameter("lightBrightness", penumbras[j]._lightBrightness);
			unshadowShader.setParameter("darkBrightness", penumbras[j]._darkBrightness);

			vertexArray[0].position = penumbras[j]._source;
			vertexArray[1].position = penumbras[j]._source + vectorNormalize(penumbras[j]._lightEdge) * shadowExtension;
			vertexArray[2].position = penumbras[j]._source + vectorNormalize(penumbras[j]._darkEdge) * shadowExtension;

			lightTempTexture.draw(vertexArray, penumbraRenderStates);
		}
	}

	/*sf::VertexArray vertexArray;

	vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

	vertexArray.resize(3);

	sf::RenderStates penumbraRenderStates;
	penumbraRenderStates.blendMode = sf::BlendMode(sf::BlendMode::OneMinusDstAlpha, sf::BlendMode::One, sf::BlendMode::Equation::Add, sf::BlendMode::Zero, sf::BlendMode::One, sf::BlendMode::Add);
	penumbraRenderStates.shader = &unshadowShader;

	{
		// Mask with penumbras
		for (int j = 0; j < penumbras.size(); j++) {
			unshadowShader.setParameter("lightBrightness", penumbras[j]._lightBrightness);
			unshadowShader.setParameter("darkBrightness", penumbras[j]._darkBrightness);

			vertexArray[0].position = penumbras[j]._source;
			vertexArray[1].position = penumbras[j]._source + vectorNormalize(penumbras[j]._lightEdge) * shadowExtension;
			vertexArray[2].position = penumbras[j]._source + vectorNormalize(penumbras[j]._darkEdge) * shadowExtension;

			vertexArray[0].texCoords = sf::Vector2f(0.0f, 1.0f);
			vertexArray[1].texCoords = sf::Vector2f(1.0f, 0.0f);
			vertexArray[2].texCoords = sf::Vector2f(0.0f, 0.0f);

			lightTempTexture.draw(vertexArray, penumbraRenderStates);
		}
	}*/

	for (int i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		int halfBoundaries = outerEdges[i]._outerBoundaryIndices.size() / 2;

		for (int j = 0; j < halfBoundaries; j++) {
			int index0 = 0 + j;
			int index1 = halfBoundaries + j;

			sf::Vector2f as = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(outerEdges[i]._outerBoundaryIndices[index0]));
			sf::Vector2f bs = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(outerEdges[i]._outerBoundaryIndices[index1]));
			sf::Vector2f ad = outerEdges[i]._outerBoundaryVectors[index0];
			sf::Vector2f bd = outerEdges[i]._outerBoundaryVectors[index1];

			sf::Vector2f intersection;

			if (rayIntersect(as, ad, bs, bd, intersection)) {
				sf::ConvexShape maskShape;

				maskShape.setPointCount(3);

				maskShape.setPoint(0, as);
				maskShape.setPoint(1, bs);
				maskShape.setPoint(2, intersection);

				maskShape.setFillColor(sf::Color::Black);

				lightTempTexture.draw(maskShape);
			}
			else {
				sf::ConvexShape maskShape;

				maskShape.setPointCount(4);

				maskShape.setPoint(0, as);
				maskShape.setPoint(1, bs);
				maskShape.setPoint(2, bs + vectorNormalize(bd) * shadowExtension);
				maskShape.setPoint(3, as + vectorNormalize(ad) * shadowExtension);

				maskShape.setFillColor(sf::Color::Black);

				lightTempTexture.draw(maskShape);
			}
		}
	}

	for (int i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		if (pLightShape->_renderLightOverShape) {
			pLightShape->_shape.setFillColor(sf::Color::White);

			lightTempTexture.draw(pLightShape->_shape, &lightOverShapeShader);
		}
		else {
			pLightShape->_shape.setFillColor(sf::Color::Black);

			lightTempTexture.draw(pLightShape->_shape);
		}
	}

	lightTempTexture.display();
}