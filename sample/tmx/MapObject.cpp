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
#include <tmx/MapObject.h>
#include <tmx/MapLayer.h>

using namespace tmx;

///--------poly segment--------///
bool MapObject::Segment::Intersects(const MapObject::Segment& segment)
{
	sf::Vector2f s1 = End - Start;
	sf::Vector2f s2 = segment.End - segment.Start;

	float s, t;
	s = (-s1.y * (Start.x - segment.Start.x) + s1.x * (Start.y - segment.Start.y)) / (-s2.x * s1.y + s1.x * s2.y);
	t = ( s2.x * (Start.y - segment.Start.y) - s2.y * (Start.x - segment.Start.x)) / (-s2.x * s1.y + s1.x * s2.y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		// Collision detected

		//point at which lines intersect - do what you will with this
		sf::Vector2f intersection(Start.x + (t * s1.x), Start.y + (t * s1.y));
		return true;
	}
	return false;
}



///------map object-------///

//ctor
MapObject::MapObject()
	 : m_visible	(true),
	 m_shape		(Rectangle),
	 m_furthestPoint(0.f)
{

}

//public
std::string MapObject::GetPropertyString(const std::string& name)
{
	if(m_properties.find(name) != m_properties.end())
		return m_properties[name];
	else
		return std::string();
}

void MapObject::SetProperty(const std::string& name, const std::string& value)
{
	m_properties[name] = value;
}

void MapObject::SetPosition(float x, float y)
{
	SetPosition(sf::Vector2f(x, y));
}

void MapObject::SetPosition(const sf::Vector2f& position)
{
	sf::Vector2f distance = position - m_position;
	Move(distance);
}

void MapObject::Move(float x, float y)
{
	Move(sf::Vector2f(x, y));
}

void MapObject::Move(const sf::Vector2f& distance)
{
	//update properties by movement amount
	m_centrePoint += distance;
	for(auto& p : m_polypoints)
		p += distance;

	m_debugShape.move(distance);

	m_AABB.left += distance.x;
	m_AABB.top += distance.y;

	//set new position
	m_position += distance;

	//if object is of type tile move vertex data
	if(m_tileQuad) m_tileQuad->Move(distance);
}

bool MapObject::Contains(sf::Vector2f point) const
{
	if(m_shape == Polyline) return false;

	//convert point to local coords
	point-= m_position;

	//TODO transform point instead

	//check if enough poly points
	if(m_polypoints.size() < 3) return false;

	//else raycast through points
	unsigned int i, j;
	bool result = false;
	for (i = 0, j = m_polypoints.size() - 1; i < m_polypoints.size(); j = i++)
	{
		if (((m_polypoints[i].y > point.y) != (m_polypoints[j].y > point.y)) &&
		(point.x < (m_polypoints[j].x - m_polypoints[i].x) * (point.y - m_polypoints[i].y)
			/ (m_polypoints[j].y - m_polypoints[i].y) + m_polypoints[i].x))
				result = !result;
	}
	return result;
}

bool MapObject::Intersects(const MapObject& object) const
{
	//check if distance between objects is less than sum of furthest points
	float distance = Helpers::Vectors::GetLength(m_centrePoint + object.m_centrePoint);
	if(distance > (m_furthestPoint + object.m_furthestPoint)) return false;

	//check intersection if either object contains a point of the other
	for(auto& p : object.m_polypoints)
		if(Contains(p + object.GetPosition())) return true;

	for(auto& p : m_polypoints)
		if(object.Contains(p + GetPosition())) return true;

	return false;
}

void MapObject::CreateDebugShape(const sf::Color& colour)
{
	if(m_polypoints.size() == 0)
	{
		std::cerr << "Unable to create debug shape, object data missing." << std::endl;
		std::cerr << "Check image file paths referenced by tmx file." << std::endl;
		return;
	}

	//reset any existing shapes incase new points have been added
	m_debugShape.Reset();

	for(const auto& p : m_polypoints)
		m_debugShape.AddVertex(sf::Vertex(p, colour));
	
	if(m_shape != Polyline) m_debugShape.CloseShape();

	m_debugShape.setPosition(m_position);

	//precompute shape values for intersection testing
	m_CalcTestValues();

	//create the AABB for quad tree testing
	m_CreateAABB();
}

void MapObject::DrawDebugShape(sf::RenderTarget& rt) const
{
	rt.draw(m_debugShape);
}

sf::Vector2f MapObject::FirstPoint() const
{
	if(!m_polypoints.empty())
		return m_polypoints[0] + m_position;
	else return sf::Vector2f();
}

sf::Vector2f MapObject::LastPoint() const
{
	if(!m_polypoints.empty())
		return(m_polypoints.back() + m_position);
	else return sf::Vector2f();
}

sf::Vector2f MapObject::CollisionNormal(const sf::Vector2f& start, const sf::Vector2f& end) const
{
	Segment trajectory(start, end);
	for(auto& s : m_polySegs)
	{
		if(trajectory.Intersects(s))
		{
			sf::Vector2f v = s.End - s.Start;
			sf::Vector2f n(v.y, -v.x);
			//invert normal if pointing in wrong direction
			float tAngle = Helpers::Vectors::GetAngle(end - start);
			float nAngle = Helpers::Vectors::GetAngle(n);
			if(nAngle - tAngle > 90.f) n =- n;

			return Helpers::Vectors::Normalize(n);
		}
	}
	sf::Vector2f rv(end - start);
	return Helpers::Vectors::Normalize(rv);
}

void MapObject::CreateSegments()
{
	if(m_polypoints.size() == 0)
	{
		std::cerr << "Unable to object segments, object data missing." << std::endl;
		std::cerr << "Check image file paths referenced by tmx file." << std::endl;
		return;
	}
	
	for(auto i = 0u; i < m_polypoints.size() - 1; i++)
	{
		m_polySegs.push_back(Segment(m_polypoints[i], m_polypoints[i + 1]));
	}
	if(m_shape != Polyline) //close shape
		m_polySegs.push_back(Segment(*(m_polypoints.end() - 1), *m_polypoints.begin()));

	std::cerr << "Added " << m_polySegs.size() << " segments to Map Object" << std::endl;
}

bool MapObject::Convex() const
{
	if (m_shape == MapObjectShape::Polyline)
		return false;
	
	bool negative = false;
	bool positive = false;

	sf::Uint16 a, b, c, n = m_polypoints.size();
	for (a = 0u; a < n; ++a)
	{
		b = (a + 1) % n;
		c = (b + 1) % n;
		
		float cross = Helpers::Vectors::Cross(m_polypoints[a], m_polypoints[b], m_polypoints[c]);
		
		if(cross < 0.f)
			negative = true;
		else if(cross > 0.f)
			positive = true;
		if (positive && negative) return false;
	}
	return true;
}

const std::vector<sf::Vector2f>& MapObject::PolyPoints()const
{
	return m_polypoints;
}

void MapObject::ReverseWinding()
{
	std::reverse(m_polypoints.begin(), m_polypoints.end());
}

void MapObject::SetQuad(std::shared_ptr<TileQuad> quad)
{
	m_tileQuad = quad;
}

//private
sf::Vector2f MapObject::m_CalcCentre() const
{
	if(m_shape == Polyline) return sf::Vector2f();

	if(m_shape == Rectangle || m_polypoints.size() < 3)
	{
		//we don't have a triangle so use bounding box
		return sf::Vector2f(m_position.x + (m_size.x / 2.f), m_position.y + (m_size.y / 2.f));
	}
	//calc centroid of poly shape
	sf::Vector2f centroid;
	float signedArea = 0.f;
	float x0 = 0.f; // Current vertex X
	float y0 = 0.f; // Current vertex Y
	float x1 = 0.f; // Next vertex X
	float y1 = 0.f; // Next vertex Y
	float a = 0.f;  // Partial signed area

	// For all vertices except last
	unsigned i;
	for(i = 0; i < m_polypoints.size() - 1; ++i)
	{
		x0 = m_polypoints[i].x;
		y0 = m_polypoints[i].y;
		x1 = m_polypoints[i + 1].x;
		y1 = m_polypoints[i + 1].y;
		a = x0 * y1 - x1 * y0;
		signedArea += a;
		centroid.x += (x0 + x1) * a;
		centroid.y += (y0 + y1) * a;
	}

	// Do last vertex
	x0 = m_polypoints[i].x;
	y0 = m_polypoints[i].y;
	x1 = m_polypoints[0].x;
	y1 = m_polypoints[0].y;
	a = x0 * y1 - x1 * y0;
	signedArea += a;
	centroid.x += (x0 + x1) * a;
	centroid.y += (y0 + y1) * a;

	signedArea *= 0.5;
	centroid.x /= (6 * signedArea);
	centroid.y /= (6 * signedArea);

	//convert to world space
	centroid += m_position;
	return centroid;
}

void MapObject::m_CalcTestValues()
{
	m_centrePoint = m_CalcCentre();
	for(auto i = m_polypoints.cbegin(); i != m_polypoints.cend(); ++i)
	{
		float length = Helpers::Vectors::GetLength(*i - m_centrePoint);
		if(m_furthestPoint < length)
		{
			m_furthestPoint = length;
			if(m_shape == Polyline) m_centrePoint = *i / 2.f;
		}
	}
	//polyline centre ought to be half way between the start point and the furthest vertex
	if(m_shape == Polyline) m_furthestPoint /= 2.f;
}

void MapObject::m_CreateAABB()
{
	if(!m_polypoints.empty())
	{
		m_AABB = sf::FloatRect(m_polypoints[0], m_polypoints[0]);
		for(auto point = m_polypoints.cbegin(); point != m_polypoints.cend(); ++point)
		{
			if(point->x < m_AABB.left) m_AABB.left = point->x;
			if(point->x > m_AABB.width) m_AABB.width = point->x;
			if(point->y < m_AABB.top) m_AABB.top = point->y;
			if(point->y > m_AABB.height) m_AABB.height = point->y;
		}

		//calc true width and height by distance between points
		m_AABB.width -= m_AABB.left;
		m_AABB.height -= m_AABB.top;

		//offset into world position
		m_AABB.left += m_position.x;
		m_AABB.top += m_position.y;


		//debug draw AABB
		//m_debugShape.append(sf::Vector2f(m_AABB.left, m_AABB.top));
		//m_debugShape.append(sf::Vector2f(m_AABB.left + m_AABB.width, m_AABB.top));
		//m_debugShape.append(sf::Vector2f(m_AABB.left + m_AABB.width, m_AABB.top + m_AABB.height));
		//m_debugShape.append(sf::Vector2f(m_AABB.left, m_AABB.top + m_AABB.height));
	}
}