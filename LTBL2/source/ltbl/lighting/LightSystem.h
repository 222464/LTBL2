#pragma once

#include <ltbl/quadtree/DynamicQuadtree.h>
#include <ltbl/lighting/LightPointEmission.h>
#include <ltbl/lighting/LightDirectionEmission.h>
#include <ltbl/lighting/LightShape.h>

#include <unordered_set>

namespace ltbl {
	class LightSystem : sf::NonCopyable {
	public:
		struct Penumbra {
			sf::Vector2f _source;
			sf::Vector2f _lightEdge;
			sf::Vector2f _darkEdge;
			float _lightBrightness;
			float _darkBrightness;

			float _distance;
		};

	private:
		sf::RenderTexture _lightTempTexture, _emissionTempTexture, _antumbraTempTexture, _compositionTexture;

		static void getPenumbrasPoint(std::vector<Penumbra> &penumbras, std::vector<int> &innerBoundaryIndices, std::vector<sf::Vector2f> &innerBoundaryVectors, std::vector<int> &outerBoundaryIndices, std::vector<sf::Vector2f> &outerBoundaryVectors, const sf::ConvexShape &shape, const sf::Vector2f &sourceCenter, float sourceRadius);
		static void getPenumbrasDirection(std::vector<Penumbra> &penumbras, std::vector<int> &innerBoundaryIndices, std::vector<sf::Vector2f> &innerBoundaryVectors, std::vector<int> &outerBoundaryIndices, std::vector<sf::Vector2f> &outerBoundaryVectors, const sf::ConvexShape &shape, const sf::Vector2f &sourceDirection, float sourceRadius, float sourceDistance);

		static void clear(sf::RenderTarget &rt, const sf::Color &color);
		
		DynamicQuadtree _shapeQuadtree;
		DynamicQuadtree _lightPointEmissionQuadtree;

		std::unordered_set<std::shared_ptr<LightPointEmission>> _pointEmissionLights;
		std::unordered_set<std::shared_ptr<LightDirectionEmission>> _directionEmissionLights;
		std::unordered_set<std::shared_ptr<LightShape>> _lightShapes;

	public:
		float _directionEmissionRange;
		float _directionEmissionRadiusMultiplier;
		sf::Color _ambientColor;

		LightSystem()
			: _directionEmissionRange(10000.0f), _directionEmissionRadiusMultiplier(1.1f), _ambientColor(sf::Color(16, 16, 16))
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
			_lightPointEmissionQuadtree.trim();
		}

		void trimShapeQuadtree() {
			_shapeQuadtree.trim();
		}

		const sf::Texture &getLightingTexture() const {
			return _compositionTexture.getTexture();
		}

		friend class LightPointEmission;
		friend class LightDirectionEmission;
		friend class LightShape;
	};
}