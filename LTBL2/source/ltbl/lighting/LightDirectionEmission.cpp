#include "LightDirectionEmission.h"

#include "LightShape.h"

#include "LightSystem.h"

#include <assert.h>

using namespace ltbl;

void LightDirectionEmission::render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &antumbraTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, float shadowExtension) {
	lightTempTexture.setView(view);

	LightSystem::clear(lightTempTexture, sf::Color::White);

	// Mask off light shape (over-masking - mask too much, reveal penumbra/antumbra afterwards)
	for (unsigned i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		// Get boundaries
		std::vector<LightSystem::Penumbra> penumbras;
		std::vector<int> innerBoundaryIndices;
		std::vector<int> outerBoundaryIndices;
		std::vector<sf::Vector2f> innerBoundaryVectors;
		std::vector<sf::Vector2f> outerBoundaryVectors;

		LightSystem::getPenumbrasDirection(penumbras, innerBoundaryIndices, innerBoundaryVectors, outerBoundaryIndices, outerBoundaryVectors, pLightShape->shape, castDirection, sourceRadius, sourceDistance);

		if (innerBoundaryIndices.size() != 2 || outerBoundaryIndices.size() != 2)
			continue;

		LightSystem::clear(antumbraTempTexture, sf::Color::White);

		antumbraTempTexture.setView(view);

		sf::ConvexShape maskShape;

		float maxDist = 0.0f;

		for (unsigned j = 0; j < pLightShape->shape.getPointCount(); j++)
			maxDist = std::max(maxDist, vectorMagnitude(view.getCenter() - pLightShape->shape.getTransform().transformPoint(pLightShape->shape.getPoint(j))));

		float totalShadowExtension = shadowExtension + maxDist;

		maskShape.setPointCount(4);

		maskShape.setPoint(0, pLightShape->shape.getTransform().transformPoint(pLightShape->shape.getPoint(innerBoundaryIndices[0])));
		maskShape.setPoint(1, pLightShape->shape.getTransform().transformPoint(pLightShape->shape.getPoint(innerBoundaryIndices[1])));
		maskShape.setPoint(2, pLightShape->shape.getTransform().transformPoint(pLightShape->shape.getPoint(innerBoundaryIndices[1])) + vectorNormalize(innerBoundaryVectors[1]) * totalShadowExtension);
		maskShape.setPoint(3, pLightShape->shape.getTransform().transformPoint(pLightShape->shape.getPoint(innerBoundaryIndices[0])) + vectorNormalize(innerBoundaryVectors[0]) * totalShadowExtension);

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
			for (unsigned j = 0; j < penumbras.size(); j++) {
				unshadowShader.setUniform("lightBrightness", penumbras[j].lightBrightness);
				unshadowShader.setUniform("darkBrightness", penumbras[j].darkBrightness);

				vertexArray[0].position = penumbras[j].source;
				vertexArray[1].position = penumbras[j].source + vectorNormalize(penumbras[j].lightEdge) * totalShadowExtension;
				vertexArray[2].position = penumbras[j].source + vectorNormalize(penumbras[j].darkEdge) * totalShadowExtension;

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

	for (unsigned i = 0; i < shapes.size(); i++) {
		LightShape* pLightShape = static_cast<LightShape*>(shapes[i]);

		if (pLightShape->renderLightOverShape) {
			pLightShape->shape.setFillColor(sf::Color::White);

			lightTempTexture.draw(pLightShape->shape);
		}
	}

	// Multiplicatively blend the light over the shadows
	sf::RenderStates lightRenderStates;
	lightRenderStates.blendMode = sf::BlendMultiply;

	lightTempTexture.setView(lightTempTexture.getDefaultView());

	lightTempTexture.draw(emissionSprite, lightRenderStates);

	lightTempTexture.display();
}
