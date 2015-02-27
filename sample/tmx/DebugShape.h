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

#ifndef DEBUG_SHAPE_H_
#define DEBUG_SHAPE_H_

#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>

class DebugShape : public sf::Drawable, public sf::Transformable
{
public:
	DebugShape();
	void AddVertex(const sf::Vertex& vert);
	void Reset();
	void CloseShape();

private:

	bool m_closed;
	sf::VertexArray m_array;
	void draw(sf::RenderTarget& rt, sf::RenderStates states)const;

};

#endif