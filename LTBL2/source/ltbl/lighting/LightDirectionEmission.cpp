#include <ltbl/lighting/LightDirectionEmission.h>

#include <ltbl/lighting/LightShape.h>

#include <ltbl/lighting/LightSystem.h>

#include <assert.h>

using namespace ltbl;

void LightDirectionEmission::render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &antumbraTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, float shadowExtension) {
	lightTempTexture.setView(view);

	LightSystem::clear(lightTempTexture, sf::Color::White);

	// Mask off light shape (over-masking - mask too much, reveal penumbra/antumbra afterwards)
	for (int i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		// Get boundaries
		std::vector<LightSystem::Penumbra> penumbras;
		std::vector<int> innerBoundaryIndices;
		std::vector<int> outerBoundaryIndices;
		std::vector<sf::Vector2f> innerBoundaryVectors;
		std::vector<sf::Vector2f> outerBoundaryVectors;

		LightSystem::getPenumbrasDirection(penumbras, innerBoundaryIndices, innerBoundaryVectors, outerBoundaryIndices, outerBoundaryVectors, pLightShape->_shape, _castDirection, _sourceRadius, _sourceDistance);

		if (innerBoundaryIndices.size() != 2 || outerBoundaryIndices.size() != 2)
			continue;

		LightSystem::clear(antumbraTempTexture, sf::Color::White);

		antumbraTempTexture.setView(view);

		sf::ConvexShape maskShape;

		float maxDist = 0.0f;

		for (int j = 0; j < pLightShape->_shape.getPointCount(); j++)
			maxDist = std::max(maxDist, vectorMagnitude(view.getCenter() - pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(j))));

		float totalShadowExtension = shadowExtension + maxDist;

		maskShape.setPointCount(4);

		maskShape.setPoint(0, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0])));
		maskShape.setPoint(1, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[1])));
		maskShape.setPoint(2, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[1])) + vectorNormalize(innerBoundaryVectors[1]) * totalShadowExtension);
		maskShape.setPoint(3, pLightShape->_shape.getTransform().transformPoint(pLightShape->_shape.getPoint(innerBoundaryIndices[0])) + vectorNormalize(innerBoundaryVectors[0]) * totalShadowExtension);

		maskShape.setFillColor(sf::Color::Black);

		antumbraTempTexture.draw(maskShape);

		sf::VertexArray vertexArray;

		vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

		vertexArray.resize(3);

		{
			sf::RenderStates states;
			states.blendMode = sf::BlendAdd;
			states.shader = &unshadowShader;

			// Unmask with penumbras
			for (int j = 0; j < penumbras.size(); j++) {
				unshadowShader.setParameter("lightBrightness", penumbras[j]._lightBrightness);
				unshadowShader.setParameter("darkBrightness", penumbras[j]._darkBrightness);

				vertexArray[0].position = penumbras[j]._source;
				vertexArray[1].position = penumbras[j]._source + vectorNormalize(penumbras[j]._lightEdge) * totalShadowExtension;
				vertexArray[2].position = penumbras[j]._source + vectorNormalize(penumbras[j]._darkEdge) * totalShadowExtension;

				vertexArray[0].texCoords = sf::Vector2f(0.0f, 1.0f);
				vertexArray[1].texCoords = sf::Vector2f(1.0f, 0.0f);
				vertexArray[2].texCoords = sf::Vector2f(0.0f, 0.0f);

				antumbraTempTexture.draw(vertexArray, states);
			}
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

	for (int i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		if (pLightShape->_renderLightOverShape) {
			pLightShape->_shape.setFillColor(sf::Color::White);

			lightTempTexture.draw(pLightShape->_shape);
		}
	}

	// Multiplicatively blend the light over the shadows
	sf::RenderStates lightRenderStates;
	lightRenderStates.blendMode = sf::BlendMultiply;

	lightTempTexture.setView(lightTempTexture.getDefaultView());

	lightTempTexture.draw(_emissionSprite, lightRenderStates);

	lightTempTexture.display();
}