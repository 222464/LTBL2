/*********************************************************************
Matt Marchant 2013 - 2014
SFML Tiled Map Loader - https://github.com/bjorn/tiled/wiki/TMX-Map-Format
						http://trederia.blogspot.com/2013/05/tiled-map-loader-for-sfml.html

The zlib license has been used to make this software fully compatible
with SFML. See http://www.sfml-dev.org/license.php

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
   you must not claim that you wrote the original software.
   If you use this software in a product, an acknowledgment
   in the product documentation would be appreciated but
   is not required.

2. Altered source versions must be plainly marked as such,
   and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
   source distribution.
*********************************************************************/

#ifndef MAPLAYER_H_
#define MAPLAYER_H_

#include <tmx/MapObject.h>
#include <memory>
#include <array>

namespace tmx
{
	class LayerSet;
	class TileQuad final
	{
		friend class LayerSet;
	public:
		typedef std::shared_ptr<TileQuad> Ptr;
		TileQuad(sf::Uint16 i0, sf::Uint16 i1, sf::Uint16 i2, sf::Uint16 i3);
		void Move(const sf::Vector2f& distance);
	private:
		std::array<sf::Uint16, 4u> m_indices;
		sf::Vector2f m_movement;
		bool m_needsUpdate;
	};

	//drawable composed of vertices representing a set of tiles on a layer
	class LayerSet final : public sf::Drawable
	{
	public:	

		LayerSet(const sf::Texture& texture);
		TileQuad::Ptr AddTile(sf::Vertex vt0, sf::Vertex vt1, sf::Vertex vt2, sf::Vertex vt3);
		void Cull(const sf::FloatRect& bounds);

	private:
		const sf::Texture& m_texture;
		mutable std::vector<TileQuad::Ptr> m_quads;
		mutable std::vector<sf::Vertex> m_vertices;
		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;

		sf::FloatRect m_boundingBox;
		void m_UpdateAABB(sf::Vector2f position, sf::Vector2f size);
		bool m_visible;
	};



	//used to query the type of layer, for example when looking for layers containing collision objects
	enum MapLayerType
	{
		Layer,
		ObjectGroup,
		ImageLayer
	};

	//represents a layer of tiles, corresponding to a tmx layer, object group or image layer
	class MapLayer final : public sf::Drawable
	{
	public:
		//used for drawing specific layers
		enum DrawType
		{
			Front,
			Back,
			Debug,
			All
		};


		MapLayer(MapLayerType layerType);
		std::string name;
		float opacity; //range 0 - 1
		bool visible;
		MapTiles tiles;
		MapObjects objects; //vector of objects if layer is object group
		MapLayerType type;
		std::map <std::string, std::string> properties;

		std::map<sf::Uint16, std::shared_ptr<LayerSet>> layerSets;
		void SetShader(const sf::Shader& shader);
		void Cull(const sf::FloatRect& bounds);

	private:
		const sf::Shader* m_shader;
		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
};

#endif //MAPLAYER_H_