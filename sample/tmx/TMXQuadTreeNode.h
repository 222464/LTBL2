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



///Creates a node used to build quad trees for spatial partitioning of MapObjects///
//Example usage: create a root node the size of the viewable area, and insert each
//available map object. Then test the root node by calling retrieve passing for example
//the AABB of a sprite. The resulting vector will contain pointers to any objects contained
//in quads which are them selves contained, or intersected, by the sprites AABB. These can
//then be collision tested.

#ifndef QUADTREE_NODE_H_
#define QUADTREE_NODE_H_

#include <tmx/MapObject.h>
#include <memory>

namespace tmx
{
	class TMXQuadTreeNode : public sf::Drawable
	{
	public:
		TMXQuadTreeNode(sf::Uint16 level = 0, const sf::FloatRect& bounds = sf::FloatRect(0.f, 0.f, 1.f, 1.f));
		virtual ~TMXQuadTreeNode(){};

		//fills vector with references to all objects which
		//appear in quads which are contained or intersect bounds.
		std::vector<MapObject*> Retrieve(const sf::FloatRect& bounds, sf::Uint16& currentDepth);
		//inserts a reference to the object into the node's object list
		void Insert(const MapObject& object);
	protected:
		//maximum objects per node before splitting
		const sf::Uint16 MAX_OBJECTS;
		//maximum number of levels to split
		const sf::Uint16 MAX_LEVELS;

		sf::Uint16 m_level;
		sf::FloatRect m_bounds;
		std::vector<MapObject*> m_objects; //objects contained in current node
		std::vector< std::shared_ptr<TMXQuadTreeNode> > m_children; //vector of child nodes
		sf::RectangleShape m_debugShape;

		//returns the index of the child node into which the givens bounds fits.
		//returns -1 if doesn't completely fit a child. Numbered anti-clockwise
		//from top right node.
		sf::Int16 m_GetIndex(const sf::FloatRect& bounds);

		//divides node by creating 4 children
		void m_Split(void);

	private:
		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};

	//specialisation of TMXQuadTreeNode for counting tree depth
	class TMXQuadTreeRoot final : public TMXQuadTreeNode
	{
	public:
		TMXQuadTreeRoot(sf::Uint16 level = 0, const sf::FloatRect& bounds = sf::FloatRect(0.f, 0.f, 1.f, 1.f))
			: TMXQuadTreeNode(level, bounds), m_depth(0u), m_searchDepth(0u){};

		//clears node and all children
		void Clear(const sf::FloatRect& newBounds);
		//retrieves all objects in quads which contains or intersect test area
		std::vector<MapObject*> Retrieve(const sf::FloatRect& bounds)
		{
			return TMXQuadTreeNode::Retrieve(bounds, m_searchDepth);
		}

	private:
		//total depth of tree, and depth reached when querying
		sf::Uint16 m_depth, m_searchDepth;
	};
};


#endif //QUADTREE_NODE_H_