#pragma once

#include "../quadtree/QuadtreeOccupant.h"

namespace ltbl {
	class LightPointEmission : public QuadtreeOccupant {
	private:
	public:
		sf::Sprite emissionSprite;
		sf::Vector2f localCastCenter;

		float sourceRadius;

		float shadowOverExtendMultiplier;

		LightPointEmission()
			: localCastCenter(0.0f, 0.0f), sourceRadius(8.0f), shadowOverExtendMultiplier(1.4f)
		{}

		sf::FloatRect getAABB() const {
			return emissionSprite.getGlobalBounds();
		}

		void render(const sf::View &view, sf::RenderTexture &lightTempTexture, sf::RenderTexture &emissionTempTexture, sf::RenderTexture &antumbraTempTexture, const std::vector<QuadtreeOccupant*> &shapes, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader);
	};
}