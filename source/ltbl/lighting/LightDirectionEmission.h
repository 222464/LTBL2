#pragma once

#include <SFML/Graphics.hpp>
#include "../quadtree/QuadtreeOccupant.h"

namespace ltbl {
	class LightDirectionEmission {
	private:
	public:
		sf::Sprite emissionSprite;
		sf::Vector2f castDirection;

		float sourceRadius;
		float sourceDistance;

		LightDirectionEmission()
			: castDirection(0.0f, 1.0f), sourceRadius(5.0f), sourceDistance(100.0f)
		{}

		void render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &antumbraTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, float shadowExtension);
	};
}