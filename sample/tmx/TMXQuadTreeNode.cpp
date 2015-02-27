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



///source for TMXQuadTreeNode class///
#include <tmx/TMXQuadTreeNode.h>

using namespace tmx;

TMXQuadTreeNode::TMXQuadTreeNode(sf::Uint16 level, const sf::FloatRect& bounds)
	: MAX_OBJECTS	(5u),
	MAX_LEVELS		(5u),
	m_level			(level),
	m_bounds		(bounds)
{ 
	m_children.reserve(4); 
	m_debugShape = sf::RectangleShape(sf::Vector2f(bounds.width, bounds.height));
	m_debugShape.setPosition(bounds.left, bounds.top);
	m_debugShape.setFillColor(sf::Color::Transparent);
	m_debugShape.setOutlineColor(sf::Color::Green);
	m_debugShape.setOutlineThickness(-2.f);

}

//public functions//
void TMXQuadTreeRoot::Clear(const sf::FloatRect& newBounds)
{
	m_objects.clear();
	m_children.clear();
	m_bounds = newBounds;
	m_debugShape.setPosition(newBounds.left, newBounds.top);
	m_debugShape.setSize(sf::Vector2f(newBounds.width, newBounds.height));

	m_searchDepth = 0u;
	m_depth = 0;
}

std::vector<MapObject*> TMXQuadTreeNode::Retrieve(const sf::FloatRect& bounds, sf::Uint16& searchDepth)
{	
	searchDepth = m_level;
	std::vector<MapObject*> foundObjects;
	sf::Int16 index = m_GetIndex(bounds);

	//recursively add objects of child node if bounds is fully contained
	if(!m_children.empty() && index != -1) 
	{
		foundObjects = m_children[index]->Retrieve(bounds, searchDepth);
	}
	else
	{
		//add all objects of child nodes which intersect test area
		for(auto& child : m_children)
		{
			if(bounds.intersects(child->m_bounds))
			{
				std::vector<MapObject*> childObjects = child->Retrieve(bounds, searchDepth);
				foundObjects.insert(foundObjects.end(), childObjects.begin(), childObjects.end());
			}
		}

	}
	//and append objects in this node
	foundObjects.insert(foundObjects.end(), m_objects.begin(), m_objects.end());
	m_debugShape.setOutlineColor(sf::Color::Red);
	return foundObjects;
}

void TMXQuadTreeNode::Insert(const MapObject& object)
{	
	//check if an object falls completely outside a node
	if(!object.GetAABB().intersects(m_bounds)) return;
	
	//if node is already split add object to corresponding child node
	//if it fits
	if(!m_children.empty())
	{
		sf::Int16 index = m_GetIndex(object.GetAABB());
		if(index != -1)
		{
			m_children[index]->Insert(object);
			return;
		}
	}
	//else add object to this node
	m_objects.push_back(const_cast<MapObject*>(&object));


	//check number of objects in this node, and split if necessary
	//adding any objects that fit to the new child node
	if(m_objects.size() > MAX_OBJECTS && m_level < MAX_LEVELS)
	{
		//split if there are no child nodes
		if(m_children.empty()) m_Split();

		sf::Uint16 i = 0;
		while(i < m_objects.size())
		{
			sf::Int16 index = m_GetIndex(m_objects[i]->GetAABB());
			if(index != -1)
			{
				m_children[index]->Insert(*m_objects[i]);
				m_objects.erase(m_objects.begin() + i);
			}
			else
			{
				i++; //we only increment i when not erasing, because erasing moves
				//everything up one index inside the vector
			}
		}
	}
}


//private functions//
void TMXQuadTreeNode::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	//recursively draw children
	for(auto& child : m_children)
		rt.draw(*child);

	rt.draw(m_debugShape);
}


void TMXQuadTreeNode::m_Split(void)
{
	const float halfWidth = m_bounds.width / 2.f;
	const float halfHeight = m_bounds.height / 2.f;
	const float x = m_bounds.left;
	const float y = m_bounds.top;
 
	m_children.push_back(std::make_shared<TMXQuadTreeNode>(m_level + 1, sf::FloatRect(x + halfWidth, y, halfWidth, halfHeight)));
	m_children.push_back(std::make_shared<TMXQuadTreeNode>(m_level + 1, sf::FloatRect(x, y, halfWidth, halfHeight)));
	m_children.push_back(std::make_shared<TMXQuadTreeNode>(m_level + 1, sf::FloatRect(x, y + halfHeight, halfWidth, halfHeight)));
	m_children.push_back(std::make_shared<TMXQuadTreeNode>(m_level + 1, sf::FloatRect(x+ halfWidth, y + halfHeight, halfWidth, halfHeight)));
}

sf::Int16 TMXQuadTreeNode::m_GetIndex(const sf::FloatRect& bounds)
{
	sf::Int16 index = -1;
	float verticalMidpoint = m_bounds.left + (m_bounds.width / 2.f);
	float horizontalMidpoint = m_bounds.top + (m_bounds.height / 2.f);
 
	//Object can completely fit within the top quadrants
	bool topQuadrant = (bounds.top < horizontalMidpoint && bounds.top + bounds.height < horizontalMidpoint);
	//Object can completely fit within the bottom quadrants
	bool bottomQuadrant = (bounds.top > horizontalMidpoint);
 
	//Object can completely fit within the left quadrants
	if(bounds.left < verticalMidpoint && bounds.left + bounds.width < verticalMidpoint)
	{
		if(topQuadrant)
		{
			index = 1;
		}
		else if(bottomQuadrant)
		{
			index = 2;
		}
	}
	//Object can completely fit within the right quadrants
	else if(bounds.left > verticalMidpoint)
	{
		if(topQuadrant)
		{
			index = 0;
		}
		else if(bottomQuadrant) 
		{
			index = 3;
		}
	}
	return index;
}