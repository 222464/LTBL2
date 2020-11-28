#pragma once

#include "../quadtree/DynamicQuadtree.h"
#include "LightPointEmission.h"
#include "LightDirectionEmission.h"
#include "LightShape.h"

#include <unordered_set>

namespace ltbl {
	class LightSystem : sf::NonCopyable {
	public:
		struct Penumbra {
			sf::Vector2f source;
			sf::Vector2f lightEdge;
			sf::Vector2f darkEdge;
			float lightBrightness;
			float darkBrightness;

			float distance;
		};

	private:
		sf::RenderTexture lightTempTexture, emissionTempTexture, antumbraTempTexture, compositionTexture;

		static void getPenumbrasPoint(std::vector<Penumbra> &penumbras, std::vector<int> &innerBoundaryIndices, std::vector<sf::Vector2f> &innerBoundaryVectors, std::vector<int> &outerBoundaryIndices, std::vector<sf::Vector2f> &outerBoundaryVectors, const sf::ConvexShape &shape, const sf::Vector2f &sourceCenter, float sourceRadius);
		static void getPenumbrasDirection(std::vector<Penumbra> &penumbras, std::vector<int> &innerBoundaryIndices, std::vector<sf::Vector2f> &innerBoundaryVectors, std::vector<int> &outerBoundaryIndices, std::vector<sf::Vector2f> &outerBoundaryVectors, const sf::ConvexShape &shape, const sf::Vector2f &sourceDirection, float sourceRadius, float sourceDistance);

		static void clear(sf::RenderTarget &rt, const sf::Color &color);
		
		DynamicQuadtree shapeQuadtree;
		DynamicQuadtree lightPointEmissionQuadtree;

		std::unordered_set<std::shared_ptr<LightPointEmission>> pointEmissionLights;
		std::unordered_set<std::shared_ptr<LightDirectionEmission>> directionEmissionLights;
		std::unordered_set<std::shared_ptr<LightShape>> lightShapes;

	public:
		float directionEmissionRange;
		float directionEmissionRadiusMultiplier;
		sf::Color ambientColor;

		LightSystem()
			: directionEmissionRange(10000.0f), directionEmissionRadiusMultiplier(1.1f), ambientColor(sf::Color(16, 16, 16))
		{}

		void create(const sf::FloatRect &rootRegion, const sf::Vector2u &imageSize, const sf::Texture &penumbraTexture, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader);

		void render(const sf::View &view, sf::Shader &unshadowShader, sf::Shader &lightOverShapeShader);

		void addShape(const std::shared_ptr<LightShape> &lightShape);

		void removeShape(const std::shared_ptr<LightShape> &lightShape);
	
		void addLight(const std::shared_ptr<LightPointEmission> &pointEmissionLight);
		void addLight(const std::shared_ptr<LightDirectionEmission> &directionEmissionLight);

		void removeLight(const std::shared_ptr<LightPointEmission> &pointEmissionLight);
		void removeLight(const std::shared_ptr<LightDirectionEmission> &directionEmissionLight);

		void trimLightPointEmissionQuadtree() {
			lightPointEmissionQuadtree.trim();
		}

		void trimShapeQuadtree() {
			shapeQuadtree.trim();
		}

		const sf::Texture &getLightingTexture() const {
			return compositionTexture.getTexture();
		}

		friend class LightPointEmission;
		friend class LightDirectionEmission;
		friend class LightShape;
	};
}