#pragma once

#include <SFML/Graphics.hpp>
#include <ltbl/quadtree/QuadtreeOccupant.h>

namespace ltbl {
	class LightDirectionEmission {
	private:
	public:
		sf::Sprite _emissionSprite;
		sf::Vector2f _castDirection;

		float _sourceRadius;
		float _sourceDistance;

		LightDirectionEmission()
			: _castDirection(0.0f, 1.0f), _sourceRadius(5.0f), _sourceDistance(100.0f)
		{}

		void render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &antumbraTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, float shadowExtension);
	};
}