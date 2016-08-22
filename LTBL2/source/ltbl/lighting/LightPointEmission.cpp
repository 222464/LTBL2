#include <ltbl/lighting/LightPointEmission.h>

#include <ltbl/lighting/LightShape.h>

#include <ltbl/lighting/LightSystem.h>

#include <iostream>

#include <assert.h>

using namespace ltbl;

void LightPointEmission::render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &emissionTempTexture, sf::RenderTexture &antumbraTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader) {
	LightSystem::clear(emissionTempTexture, sf::Color::Black);

	emissionTempTexture.setView(view);

	emissionTempTexture.draw(_emissionSprite);

	emissionTempTexture.display();
	
	LightSystem::clear(lightTempTexture, sf::Color::Black);

	lightTempTexture.setView(view);

	lightTempTexture.draw(_emissionSprite);

	sf::Transform t;
	t.translate(_emissionSprite.getPosition());
	t.rotate(_emissionSprite.getRotation());
	t.scale(_emissionSprite.getScale());

	sf::Vector2f castCenter = t.transformPoint(_localCastCenter);

	float shadowExtension = _shadowOverExtendMultiplier * (getAABB().width + getAABB().height);

	struct OuterEdges {
		std::vector<int> _outerBoundaryIndices;
		std::vector<sf::Vector2f> _outerBoundaryVectors;
	};

	std::vector<OuterEdges> outerEdges(shapes.size());

	// Mask off light shape (over-masking - mask too much, reveal penumbra/antumbra afterwards)
	for (unsigned i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		// Get boundaries
		std::vector<int> innerBoundaryIndices;
		std::vector<sf::Vector2f> innerBoundaryVectors;
		std::vector<LightSystem::Penumbra> penumbras;

		LightSystem::getPenumbrasPoint(penumbras, innerBoundaryIndices, innerBoundaryVectors, outerEdges[i]._outerBoundaryIndices, outerEdges[i]._outerBoundaryVectors, pLightShape->_shape, castCenter, _sourceRadius);

		if (innerBoundaryIndices.size() != 2 || outerEdges[i]._outerBoundaryIndices.size() != 2)
			continue;

		// Render shape
		if (!pLightShape->_renderLightOverShape) {
			pLightShape->_shape.setFillColor(sf::Color::Black);

			lightTempTexture.draw(pLightShape->_shape);
		}

		sf::RenderStates maskRenderStates;
		maskRenderStates.blendMode = sf::BlendNone;

		sf::Vector2f as = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(outerEdges[i]._outerBoundaryIndices[0]));
		sf::Vector2f bs = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(outerEdges[i]._outerBoundaryIndices[1]));
		sf::Vector2f ad = outerEdges[i]._outerBoundaryVectors[0];
		sf::Vector2f bd = outerEdges[i]._outerBoundaryVectors[1];

		sf::Vector2f intersectionOuter;

		// Handle antumbras as a seperate case
		if (rayIntersect(as, ad, bs, bd, intersectionOuter)) {
			sf::Vector2f asi = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0]));
			sf::Vector2f bsi = pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[1]));
			sf::Vector2f adi = innerBoundaryVectors[0];
			sf::Vector2f bdi = innerBoundaryVectors[1];

			LightSystem::clear(antumbraTempTexture, sf::Color::White);

			antumbraTempTexture.setView(view);

			sf::Vector2f intersectionInner;

			if (rayIntersect(asi, adi, bsi, bdi, intersectionInner)) {
				sf::ConvexShape maskShape;

				maskShape.setPointCount(3);

				maskShape.setPoint(0, asi);
				maskShape.setPoint(1, bsi);
				maskShape.setPoint(2, intersectionInner);

				maskShape.setFillColor(sf::Color::Black);

				antumbraTempTexture.draw(maskShape);	
			}
			else {
				sf::ConvexShape maskShape;

				maskShape.setPointCount(4);

				maskShape.setPoint(0, asi);
				maskShape.setPoint(1, bsi);
				maskShape.setPoint(2, bsi + vectorNormalize(bdi) * shadowExtension);
				maskShape.setPoint(3, asi + vectorNormalize(adi) * shadowExtension);

				maskShape.setFillColor(sf::Color::Black);

				antumbraTempTexture.draw(maskShape);
			}

			// Add light back for antumbra/penumbras
			sf::VertexArray vertexArray;

			vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

			vertexArray.resize(3);

			sf::RenderStates penumbraRenderStates;
			penumbraRenderStates.blendMode = sf::BlendAdd;
			penumbraRenderStates.shader = &unshadowShader;

			// Unmask with penumbras
			for (unsigned j = 0; j < penumbras.size(); j++) {
				unshadowShader.setParameter("lightBrightness", penumbras[j]._lightBrightness);
				unshadowShader.setParameter("darkBrightness", penumbras[j]._darkBrightness);

				vertexArray[0].position = penumbras[j]._source;
				vertexArray[1].position = penumbras[j]._source + vectorNormalize(penumbras[j]._lightEdge) * shadowExtension;
				vertexArray[2].position = penumbras[j]._source + vectorNormalize(penumbras[j]._darkEdge) * shadowExtension;

				vertexArray[0].texCoords = sf::Vector2f(0.0f, 1.0f);
				vertexArray[1].texCoords = sf::Vector2f(1.0f, 0.0f);
				vertexArray[2].texCoords = sf::Vector2f(0.0f, 0.0f);

				antumbraTempTexture.draw(vertexArray, penumbraRenderStates);
			}

			antumbraTempTexture.display();

			// Multiply back to lightTempTexture
			sf::RenderStates antumbraRenderStates;
			antumbraRenderStates.blendMode = sf::BlendMultiply;

			sf::Sprite s;

			s.setTexture(antumbraTempTexture.getTexture());

			lightTempTexture.setView(lightTempTexture.getDefaultView());

			lightTempTexture.draw(s, antumbraRenderStates);

			lightTempTexture.setView(view);
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

			sf::VertexArray vertexArray;

			vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

			vertexArray.resize(3);

			sf::RenderStates penumbraRenderStates;
			penumbraRenderStates.blendMode = sf::BlendMultiply;
			penumbraRenderStates.shader = &unshadowShader;

			// Unmask with penumbras
			for (unsigned j = 0; j < penumbras.size(); j++) {
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
		}
	}

	for (unsigned i = 0; i < shapes.size(); i++) {
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
