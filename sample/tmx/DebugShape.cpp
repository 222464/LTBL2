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

#include <tmx/DebugShape.h>
#include <SFML/Graphics/RenderTarget.hpp>

DebugShape::DebugShape()
: m_array	(sf::LinesStrip),
m_closed	(false)
{}

//public
void DebugShape::AddVertex(const sf::Vertex& v)
{
	if(m_closed)
	{
		sf::Uint16 i = m_array.getVertexCount() - 1;
		sf::Vertex vt = m_array[i];
		m_array[i] = v;
		m_array.append(vt);
	}
	else
	{
		m_array.append(v);
	}
}

void DebugShape::Reset()
{
	m_array.clear();
}

void DebugShape::CloseShape()
{
	if(m_array.getVertexCount())
	{
		m_array.append(m_array[0]);
		m_closed = true;
	}
}

//private
void DebugShape::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();
	rt.draw(m_array, states);
}
