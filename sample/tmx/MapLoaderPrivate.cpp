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

#include <tmx/MapLoader.h>
#include <sstream>
#include <zlib.h>
#include <cstring>

using namespace tmx;

void MapLoader::m_Unload()
{
	m_tilesetTextures.clear();
	m_tileInfo.clear();
	m_layers.clear();
	m_imageLayerTextures.clear();
	m_mapLoaded = false;
	m_quadTreeAvailable = false;
	m_failedImage = false;
}

void MapLoader::m_SetDrawingBounds(const sf::View& view)
{
	if(view.getCenter() != m_lastViewPos)
	{
		sf::FloatRect bounds;
		bounds.left = view.getCenter().x - (view.getSize().x / 2.f);
		bounds.top = view.getCenter().y - (view.getSize().y / 2.f);
		bounds.width = view.getSize().x;
		bounds.height = view.getSize().y;

		//add a tile border to prevent gaps appearing
		bounds.left -= static_cast<float>(m_tileWidth);
		bounds.top -= static_cast<float>(m_tileHeight);
		bounds.width += static_cast<float>(m_tileWidth * 2);
		bounds.height += static_cast<float>(m_tileHeight * 2);
		m_bounds = bounds;
	}
	m_lastViewPos = view.getCenter();
}

bool MapLoader::m_ParseMapNode(const pugi::xml_node& mapNode)
{
	//parse tile properties
	if(!(m_width = mapNode.attribute("width").as_int()) ||
		!(m_height = mapNode.attribute("height").as_int()) ||
		!(m_tileWidth = mapNode.attribute("tilewidth").as_int()) ||
		!(m_tileHeight = mapNode.attribute("tileheight").as_int()))
	{
		std::cerr << "Invalid tile size found, check map data. Map not loaded." << std::endl;
		return false;
	}

	//parse orientation property
	std::string orientation = mapNode.attribute("orientation").as_string();

	if(orientation == "orthogonal")
	{
		m_orientation = Orthogonal;
	}
	else if(orientation == "isometric")
	{
		m_orientation = Isometric;
		m_tileRatio = static_cast<float>(m_tileWidth) / static_cast<float>(m_tileHeight);
	}
	else
	{
		std::cerr << "Map orientation " << orientation << " not currently supported. Map not loaded." << std::endl;
		return false;
	}

	//parse any map properties
	if(pugi::xml_node propertiesNode = mapNode.child("properties"))
	{
		pugi::xml_node propertyNode = propertiesNode.child("property");
		while(propertyNode)
		{
			std::string name = propertyNode.attribute("name").as_string();
			std::string value = propertyNode.attribute("value").as_string();
			m_properties[name] = value;
			propertyNode = propertyNode.next_sibling("property");
			std::cerr << "Added map property " << name << " with value " << value << std::endl;
		}
	}

	return true;
}

bool MapLoader::m_ParseTileSets(const pugi::xml_node& mapNode)
{
	pugi::xml_node tileset;
	if(!(tileset = mapNode.child("tileset")))
	{
		std::cerr << "No tile sets found." << std::endl;
		return false;
	}
	std::cerr << "Caching image files, please wait..." << std::endl;

	//empty vertex tile
	m_tileInfo.push_back(TileInfo());

	//parse tile sets in order so GUIDs match index
	while(tileset)
	{
		//if source attrib parse external tsx
		if(tileset.attribute("source"))
		{
			//try loading tsx
			std::string file = m_FileFromPath(tileset.attribute("source").as_string());
			std::string path;
			pugi::xml_document tsxDoc;
			pugi::xml_parse_result result;
			
			for(auto& p : m_searchPaths)
			{
				path = p + file;
				result = tsxDoc.load_file(path.c_str());
				if(result) break;
			}
			if(!result)
			{
				std::cerr << "Failed to open external tsx document: " << path << std::endl;
				std::cerr << "Reason: " << result.description() << std::endl;
				std::cerr << "Make sure to add any external paths with AddSearchPath()" << std::endl;
				m_Unload(); //purge any partially loaded data
				return false;
			}
			
			//try parsing tileset node
			pugi::xml_node ts = tsxDoc.child("tileset");

			if(!m_ProcessTiles(ts)) return false;
		}
		else //try for tmx map file data
		{
			if(!m_ProcessTiles(tileset)) return false;
		}

		//move on to next tileset node
		tileset = tileset.next_sibling("tileset");
	}

	return true;
}

bool MapLoader::m_ProcessTiles(const pugi::xml_node& tilesetNode)
{
	sf::Uint16 tileWidth, tileHeight, spacing, margin;

	//try and parse tile sizes
	if(!(tileWidth = tilesetNode.attribute("tilewidth").as_int()) ||
		!(tileHeight = tilesetNode.attribute("tileheight").as_int()))
	{
		std::cerr << "Invalid tileset data found. Map not loaded." << std::endl;
		m_Unload();
		return false;
	}
	spacing = (tilesetNode.attribute("spacing")) ? tilesetNode.attribute("spacing").as_int() : 0u;
	margin = (tilesetNode.attribute("margin")) ? tilesetNode.attribute("margin").as_int() : 0u;

	//try parsing image node
	pugi::xml_node imageNode;
	if(!(imageNode = tilesetNode.child("image")) || !imageNode.attribute("source"))
	{
		std::cerr << "Missing image data in tmx file. Map not loaded." << std::endl;
		m_Unload();
		return false;
	}

	//process image from disk
	std::string imageName = m_FileFromPath(imageNode.attribute("source").as_string());
	sf::Image sourceImage = m_LoadImage(imageName);
	if(m_failedImage)
	{
		std::cerr << "Failed to load image " << imageName << std::endl;
		std::cerr << "Please check image exists and add any external paths with AddSearchPath()" << std::endl;
		return false;
	}

	//add transparency mask from colour if it exists
	if(imageNode.attribute("trans"))
		sourceImage.createMaskFromColor(m_ColourFromHex(imageNode.attribute("trans").as_string()));

	//store image as a texture for drawing with vertex array
	sf::Texture tileset;
	tileset.loadFromImage(sourceImage);
	m_tilesetTextures.push_back(tileset);

	//parse offset node if it exists - TODO store somewhere tileset info can be referenced
	sf::Vector2u offset;
	if(pugi::xml_node offsetNode = tilesetNode.child("tileoffset"))
	{
		offset.x = (offsetNode.attribute("x")) ? offsetNode.attribute("x").as_uint() : 0u;
		offset.y = (offsetNode.attribute("y")) ? offsetNode.attribute("y").as_uint() : 0u;
	}
	//TODO parse any tile properties and store with offset above

	//slice into tiles
	int columns = (sourceImage.getSize().x - margin) / (tileWidth + spacing);
	int rows = (sourceImage.getSize().y - margin) / (tileHeight + spacing);

	for (int y = 0; y < rows; y++)
	{
		for (int x = 0; x < columns; x++)
		{
			sf::IntRect rect; //must account for any spacing or margin on the tileset
			rect.top = y * (tileHeight + spacing);
			rect.top += margin;
			rect.height = tileHeight;
			rect.left = x * (tileWidth + spacing);
			rect.left += margin;
			rect.width = tileWidth;

			//store texture coords and tileset index for vertex array
			m_tileInfo.push_back(TileInfo(rect,
				sf::Vector2f(static_cast<float>(rect.width), static_cast<float>(rect.height)),
				m_tilesetTextures.size() - 1u));
		}
	}

	std::cerr << "Processed " << imageName << std::endl;
	return true;
}

bool MapLoader::m_ParseLayer(const pugi::xml_node& layerNode)
{
	std::cerr << "Found standard map layer " << layerNode.attribute("name").as_string() << std::endl;

	MapLayer layer(Layer);
	if(layerNode.attribute("name")) layer.name = layerNode.attribute("name").as_string();
	if(layerNode.attribute("opacity")) layer.opacity = layerNode.attribute("opacity").as_float();
	if(layerNode.attribute("visible")) layer.visible = layerNode.attribute("visible").as_bool();

	pugi::xml_node dataNode;
	if(!(dataNode = layerNode.child("data")))
	{
		std::cerr << "Layer data missing or corrupt. Map not loaded." << std::endl;
		return false;
	}
	//decode and decompress data first if necessary. See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#data
	//for explanation of bytestream retrieved when using compression
	if(dataNode.attribute("encoding"))
	{
		std::string encoding = dataNode.attribute("encoding").as_string();
		std::string data = dataNode.text().as_string();

		if(encoding == "base64")
		{
			std::cerr << "Found Base64 encoded layer data, decoding..." << std::endl;
			//remove any newlines or white space created by tab spaces in document
			std::stringstream ss;
			ss << data;
			ss >> data;
			data = base64_decode(data);

			//calc the expected size of the uncompressed string
			int expectedSize = m_width * m_height * 4; //number of tiles * 4 bytes = 32bits / tile
			std::vector<unsigned char>byteArray; //to hold decompressed data as bytes
			byteArray.reserve(expectedSize);

			//check for compression (only used with base64 encoded data)
			if(dataNode.attribute("compression"))
			{
				std::string compression	= dataNode.attribute("compression").as_string();
				std::cerr << "Found " << compression << " compressed layer data, decompressing..." << std::endl;

				//decompress with zlib
				int dataSize = data.length() * sizeof(unsigned char);
				if(!m_Decompress(data.c_str(), byteArray, dataSize, expectedSize))
				{
					std::cerr << "Failed to decompress map data. Map not loaded." << std::endl;
					return false;
				}
			}
			else //uncompressed
			{
				byteArray.insert(byteArray.end(), data.begin(), data.end());
			}

			//extract tile GIDs using bitshift (See https://github.com/bjorn/tiled/wiki/TMX-Map-Format#data) and add the tiles to layer
			sf::Uint16 x, y;
			x = y = 0;
			for(int i = 0; i < expectedSize - 3; i +=4)
			{
                sf::Uint32 tileGID = byteArray[i] | byteArray[i + 1] << 8 | byteArray[i + 2] << 16 | byteArray[i + 3] << 24;
//                sf::Uint32 tileGID = resolveRotation(&byteArray[i]);


				m_AddTileToLayer(layer, x, y, tileGID);

				x++;
				if(x == m_width)
				{
					x = 0;
					y++;
				}
			}
		}
		else if(encoding == "csv")
		{
			std::cerr << "CSV encoded layer data found." << std::endl;

            std::vector<sf::Uint32> tileGIDs;
			std::stringstream datastream(data);

			//parse csv string into vector of IDs
            sf::Uint32 i;
			while (datastream >> i)
			{

                tileGIDs.push_back(i);
				if(datastream.peek() == ',')
					datastream.ignore();
			}

			//create tiles from IDs
			sf::Uint16 x, y;
			x = y = 0;
			for(unsigned int i = 0; i < tileGIDs.size(); i++)
			{
//                sf::Uint32 gid=resolveRotation(tileGIDs[i]);
                m_AddTileToLayer(layer, x, y, tileGIDs[i]);
				x++;
				if(x == m_width)
				{
					x = 0;
					y++;
				}
			}
		}
		else
		{
			std::cerr << "Unsupported encoding of layer data found. Map not Loaded." << std::endl;
			return false;
		}
	}
	else //unencoded
	{
		std::cerr << "Found unencoded data." << std::endl;
		pugi::xml_node tileNode;
		if(!(tileNode = dataNode.child("tile")))
		{
			std::cerr << "No tile data found. Map not loaded." << std::endl;
			return false;
		}

		sf::Uint16 x, y;
		x = y = 0;
		while(tileNode)
		{
            sf::Uint32 gid = tileNode.attribute("gid").as_uint();

//            gid=resolveRotation(gid);


			m_AddTileToLayer(layer, x, y, gid);

			tileNode = tileNode.next_sibling("tile");
			x++;
			if(x == m_width)
			{
				x = 0;
				y++;
			}
		}
	}

	//parse any layer properties
	if(pugi::xml_node propertiesNode = layerNode.child("properties"))
		m_ParseLayerProperties(propertiesNode, layer);

	//convert layer tile coords to isometric if needed
	if(m_orientation == Isometric) m_SetIsometricCoords(layer);

	m_layers.push_back(layer);
	return true;
}

std::vector<unsigned char> MapLoader::m_IntToBytes(sf::Uint32 paramInt)
{
     std::vector<unsigned char> arrayOfByte(4);
     for (int i = 0; i < 4; i++)
         arrayOfByte[i] = (paramInt >> (i * 8));
     return arrayOfByte;
}

std::pair<sf::Uint32, std::bitset<3> > MapLoader::m_ResolveRotation(sf::Uint32 gid)
{
    const unsigned FLIPPED_HORIZONTALLY_FLAG = 0x80000000;
    const unsigned FLIPPED_VERTICALLY_FLAG   = 0x40000000;
    const unsigned FLIPPED_DIAGONALLY_FLAG   = 0x20000000;

    std::vector<unsigned char> bytes = m_IntToBytes(gid);
    sf::Uint32 tileGID = bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24;

    bool flipped_diagonally = (tileGID & FLIPPED_DIAGONALLY_FLAG);
    bool flipped_horizontally = (tileGID & FLIPPED_HORIZONTALLY_FLAG);
    bool flipped_vertically = (tileGID & FLIPPED_VERTICALLY_FLAG);

    std::bitset<3> b;
    b.set(0,flipped_vertically);
    b.set(1,flipped_horizontally);
    b.set(2,flipped_diagonally);

    tileGID &= ~(FLIPPED_HORIZONTALLY_FLAG |
                            FLIPPED_VERTICALLY_FLAG |
                            FLIPPED_DIAGONALLY_FLAG);
    return std::pair<sf::Uint32, std::bitset<3> >(tileGID,b);
}

void MapLoader::m_FlipY(sf::Vector2f *v0, sf::Vector2f *v1, sf::Vector2f *v2, sf::Vector2f *v3)
{
    //Flip Y
    sf::Vector2f tmp = *v0;
    v0->y = v2->y;
    v1->y = v2->y;
    v2->y = tmp.y ;
    v3->y = v2->y  ;
}

void MapLoader::m_FlipX(sf::Vector2f *v0, sf::Vector2f *v1, sf::Vector2f *v2, sf::Vector2f *v3)
{
    //Flip X
    sf::Vector2f tmp = *v0;
    v0->x = v1->x;
    v1->x = tmp.x;
    v2->x = v3->x;
    v3->x = v0->x ;
}

void MapLoader::m_FlipD(sf::Vector2f *v0, sf::Vector2f *v1, sf::Vector2f *v2, sf::Vector2f *v3)
{
    //Diaganol flip
    sf::Vector2f tmp = *v1;
    v1->x = v3->x;
    v1->y = v3->y;
    v3->x = tmp.x;
    v3->y = tmp.y;
}

void MapLoader::m_DoFlips(std::bitset<3> bits, sf::Vector2f *v0, sf::Vector2f *v1, sf::Vector2f *v2, sf::Vector2f *v3)
{
    //000 = no change
    //001 = vertical = swap y axis
    //010 = horizontal = swap x axis
    //011 = horiz + vert = swap both axes = horiz+vert = rotate 180 degrees
    //100 = diag = rotate 90 degrees right and swap x axis
    //101 = diag+vert = rotate 270 degrees right
    //110 = horiz+diag = rotate 90 degrees right
    //111 = horiz+vert+diag = rotate 90 degrees right and swap y axis

    if(!bits.test(0) && !bits.test(1) && !bits.test(2))
    {
        //Shortcircuit tests for nothing to do
        return;
    }
    else if(bits.test(0) && !bits.test(1) && !bits.test(2))
    {
        //001
        m_FlipY(v0,v1,v2,v3);
    }
    else if(!bits.test(0) && bits.test(1) && !bits.test(2))
    {
        //010
        m_FlipX(v0,v1,v2,v3);
    }
    else if(bits.test(0) && bits.test(1) && !bits.test(2))
    {
        //011
        m_FlipY(v0,v1,v2,v3);
        m_FlipX(v0,v1,v2,v3);
    }
    else if(!bits.test(0) && !bits.test(1) && bits.test(2))
    {
        //100
        m_FlipD(v0,v1,v2,v3);
    }
    else if(bits.test(0) && !bits.test(1) && bits.test(2))
    {
        //101
        m_FlipX(v0,v1,v2,v3);
        m_FlipD(v0,v1,v2,v3);


    }
    else if(!bits.test(0) && bits.test(1) && bits.test(2))
    {
        //110
        m_FlipY(v0,v1,v2,v3);
        m_FlipD(v0,v1,v2,v3);

    }
    else if(bits.test(0) && bits.test(1) && bits.test(2))
    {
        //111
        m_FlipY(v0,v1,v2,v3);
        m_FlipX(v0,v1,v2,v3);
        m_FlipD(v0,v1,v2,v3);
    }
}

TileQuad::Ptr MapLoader::m_AddTileToLayer(MapLayer& layer, sf::Uint16 x, sf::Uint16 y, sf::Uint32 gid, const sf::Vector2f& offset)
{
	sf::Uint8 opacity = static_cast<sf::Uint8>(255.f * layer.opacity);
	sf::Color colour = sf::Color(255u, 255u, 255u, opacity);

    //Get bits and tile id
    std::pair<sf::Uint32, std::bitset<3> > idAndFlags = m_ResolveRotation(gid);
    gid = idAndFlags.first;

	//update the layer's tile set(s)
    sf::Vertex v0, v1, v2, v3;

	//applying half pixel trick avoids artifacting when scrolling
	v0.texCoords = m_tileInfo[gid].Coords[0] + sf::Vector2f(0.5f, 0.5f);
	v1.texCoords = m_tileInfo[gid].Coords[1] + sf::Vector2f(-0.5f, 0.5f);
	v2.texCoords = m_tileInfo[gid].Coords[2] + sf::Vector2f(-0.5f, -0.5f);
	v3.texCoords = m_tileInfo[gid].Coords[3] + sf::Vector2f(0.5f, -0.5f);

    //flip texture coordinates according to bits set
    m_DoFlips(idAndFlags.second,&v0.texCoords,&v1.texCoords,&v2.texCoords,&v3.texCoords);

	v0.position = sf::Vector2f(static_cast<float>(m_tileWidth * x), static_cast<float>(m_tileHeight * y));
	v1.position = sf::Vector2f(static_cast<float>(m_tileWidth * x) + m_tileInfo[gid].Size.x, static_cast<float>(m_tileHeight * y));
	v2.position = sf::Vector2f(static_cast<float>(m_tileWidth * x) + m_tileInfo[gid].Size.x, static_cast<float>(m_tileHeight * y) + m_tileInfo[gid].Size.y);
	v3.position = sf::Vector2f(static_cast<float>(m_tileWidth * x), static_cast<float>(m_tileHeight * y) + m_tileInfo[gid].Size.y);

	//offset tiles with size not equal to map grid size
	sf::Uint16 tileHeight = static_cast<sf::Uint16>(m_tileInfo[gid].Size.y);
	if(tileHeight != m_tileHeight)
	{
		float diff = static_cast<float>(m_tileHeight - tileHeight);
		v0.position.y += diff;
		v1.position.y += diff;
		v2.position.y += diff;
		v3.position.y += diff;
	}

	//adjust position for isometric maps
	if(m_orientation == Isometric)
	{
		sf::Vector2f isoOffset(-static_cast<float>(x * (m_tileWidth / 2u)), static_cast<float>(x * (m_tileHeight / 2u)));
		isoOffset.x -= static_cast<float>(y * (m_tileWidth / 2u));
		isoOffset.y -= static_cast<float>(y * (m_tileHeight / 2u));
		isoOffset.x -= static_cast<float>(m_tileWidth / 2u);
		isoOffset.y += static_cast<float>(m_tileHeight / 2u);

		v0.position += isoOffset;
		v1.position += isoOffset;
		v2.position += isoOffset;
		v3.position += isoOffset;
	}

	v0.color = colour;
	v1.color = colour;
	v2.color = colour;
	v3.color = colour;

	v0.position += offset;
	v1.position += offset;
	v2.position += offset;
	v3.position += offset;

	sf::Uint16 id = m_tileInfo[gid].TileSetId;
	if(layer.layerSets.find(id) == layer.layerSets.end())
	{
		//create a new layerset for texture
		layer.layerSets[id] = std::make_shared<LayerSet>(m_tilesetTextures[id]);
	}

	//add tile to set
	return layer.layerSets[id]->AddTile(v0, v1, v2, v3);
}

bool MapLoader::m_ParseObjectgroup(const pugi::xml_node& groupNode)
{
	std::cerr << "Found object layer " << groupNode.attribute("name").as_string() << std::endl;

	pugi::xml_node objectNode;
	if(!(objectNode = groupNode.child("object")))
	{
		std::cerr << "Object group contains no objects" << std::endl;
		return true;
	}

	//add layer to map layers
	MapLayer layer(ObjectGroup);

	layer.name = groupNode.attribute("name").as_string();
	if(groupNode.attribute("opacity")) layer.opacity = groupNode.attribute("opacity").as_float();
	if(pugi::xml_node propertiesNode = groupNode.child("properties"))
		m_ParseLayerProperties(propertiesNode, layer);
	//NOTE we push the layer onto the vector at the end of the function in case we add any objects
	//with tile data to the layer's tiles property

	//parse all object nodes into MapObjects
	while(objectNode)
	{
		if(!objectNode.attribute("x") || !objectNode.attribute("y"))
		{
			std::cerr << "Object missing position data. Map not loaded." << std::endl;
			m_Unload();
			return false;
		}
		MapObject object;

		//set position
		sf::Vector2f position(objectNode.attribute("x").as_float(),
											objectNode.attribute("y").as_float());
		position = IsometricToOrthogonal(position);
		object.SetPosition(position);

		//set size if specified
		if(objectNode.attribute("width") && objectNode.attribute("height"))
		{
			sf::Vector2f size(objectNode.attribute("width").as_float(),
							objectNode.attribute("height").as_float());
			if(objectNode.child("ellipse"))
			{
				//add points to make ellipse
				const float x = size.x / 2.f;
				const float y = size.y / 2.f;
				const float tau = 6.283185f;
				const float step = tau / 16.f; //number of points to make up ellipse
				for(float angle = 0.f; angle < tau; angle += step)
				{
					sf::Vector2f point(x + x * cos(angle), y + y * sin(angle));
					object.AddPoint(IsometricToOrthogonal(point));
				}

				if (size.x == size.y) object.SetShapeType(Circle);
				else object.SetShapeType(Ellipse);
			}
			else //add points for rectangle to use in intersection testing
			{
				object.AddPoint(IsometricToOrthogonal(sf::Vector2f()));
				object.AddPoint(IsometricToOrthogonal(sf::Vector2f(size.x, 0.f)));
				object.AddPoint(IsometricToOrthogonal(sf::Vector2f(size.x, size.y)));
				object.AddPoint(IsometricToOrthogonal(sf::Vector2f(0.f, size.y)));
			}
			object.SetSize(size);
		}
		//else parse poly points
		else if(objectNode.child("polygon") || objectNode.child("polyline"))
		{
			if(objectNode.child("polygon"))
				object.SetShapeType(Polygon);
			else object.SetShapeType(Polyline);

			//split coords into pairs
			if(objectNode.first_child().attribute("points"))
			{
				std::cerr << "Processing poly shape points..." << std::endl;
				std::string pointlist = objectNode.first_child().attribute("points").as_string();
				std::stringstream stream(pointlist);
				std::vector<std::string> points;
				std::string pointstring;
				while(std::getline(stream, pointstring, ' '))
					points.push_back(pointstring);

				//parse each pair into sf::vector2i
				for(unsigned int i = 0; i < points.size(); i++)
				{
					std::vector<float> coords;
					std::stringstream coordstream(points[i]);

					float j;
					while (coordstream >> j)
					{
						coords.push_back(j);
						if(coordstream.peek() == ',')
							coordstream.ignore();
					}
					object.AddPoint(IsometricToOrthogonal(sf::Vector2f(coords[0], coords[1])));
				}
			}
			else
			{
				std::cerr << "Points for polygon or polyline object are missing" << std::endl;
			}
		}
		else if(!objectNode.attribute("gid")) //invalid  attributes
		{
			std::cerr << "Objects with no parameters found, skipping.." << std::endl;
			objectNode = objectNode.next_sibling("object");
			continue;
		}

		//parse object node property values
		if(pugi::xml_node propertiesNode = objectNode.child("properties"))
		{
			pugi::xml_node propertyNode = propertiesNode.child("property");
			while(propertyNode)
			{
				std::string name = propertyNode.attribute("name").as_string();
				std::string value = propertyNode.attribute("value").as_string();
				object.SetProperty(name, value);

				std::cerr << "Set object property " << name << " with value " << value << std::endl;
				propertyNode = propertyNode.next_sibling("property");
			}
		}

		//set object properties
		if(objectNode.attribute("name")) object.SetName(objectNode.attribute("name").as_string());
		if(objectNode.attribute("type")) object.SetType(objectNode.attribute("type").as_string());
		//if(objectNode.attribute("rotation")) {} //TODO handle rotation attribute
		if(objectNode.attribute("visible")) object.SetVisible(objectNode.attribute("visible").as_bool());
		if(objectNode.attribute("gid"))
		{		
			sf::Uint32 gid = objectNode.attribute("gid").as_int();

			std::cerr << "Found object with tile GID " << gid << std::endl;

			object.Move(0.f, static_cast<float>(-m_tileHeight)); //offset for tile origins being at the bottom in Tiled
			const sf::Uint16 x = static_cast<sf::Uint16>(object.GetPosition().x / m_tileWidth);
			const sf::Uint16 y = static_cast<sf::Uint16>(object.GetPosition().y / m_tileHeight);
			
			sf::Vector2f offset(object.GetPosition().x - (x * m_tileWidth), (object.GetPosition().y - (y * m_tileHeight)));
			object.SetQuad(m_AddTileToLayer(layer, x, y, gid, offset));
			object.SetShapeType(Tile);

			TileInfo info = m_tileInfo[gid];
			//create bounding poly
			float width = static_cast<float>(info.Size.x);
			float height = static_cast<float>(info.Size.y);

			object.AddPoint(sf::Vector2f());
			object.AddPoint(sf::Vector2f(width, 0.f));
			object.AddPoint(sf::Vector2f(width, height));
			object.AddPoint(sf::Vector2f(0.f, height));
			object.SetSize(sf::Vector2f(width, height));

			//move object if tile not map tile size
			if(info.Size.y != m_tileHeight)
				object.Move(0.f, static_cast<float>(m_tileHeight - info.Size.y) / 2.f);
		}
		object.SetParent(layer.name);

		//call objects create debug shape function with colour / opacity
		sf::Color debugColour;
		if(groupNode.attribute("color"))
		{
			std::string colour = groupNode.attribute("color").as_string();
			//crop leading hash and pop the last (duplicated) char
			std::remove(colour.begin(), colour.end(), '#');
			colour.pop_back();
			debugColour = m_ColourFromHex(colour.c_str());
		}
		else debugColour = sf::Color(127u, 127u, 127u);
		debugColour.a = static_cast<sf::Uint8>(255.f * layer.opacity);
		object.CreateDebugShape(debugColour);

		//creates line segments from any available points
		object.CreateSegments();

		//add objects to vector
		layer.objects.push_back(object);
		objectNode = objectNode.next_sibling("object");
	}

	m_layers.push_back(layer); //do this last
	std::cerr << "Processed " << layer.objects.size() << " objects" << std::endl;
	return true;
}

bool MapLoader::m_ParseImageLayer(const pugi::xml_node& imageLayerNode)
{
	std::cerr << "Found image layer " << imageLayerNode.attribute("name").as_string() << std::endl;

	pugi::xml_node imageNode;
	//load image
	if(!(imageNode = imageLayerNode.child("image")) || !imageNode.attribute("source"))
	{
		std::cerr << "Image layer " << imageLayerNode.attribute("name").as_string() << " missing image source property. Map not loaded." << std::endl;
		return false;
	}

	std::string imageName = imageNode.attribute("source").as_string();
	sf::Image image = m_LoadImage(imageName);
	if(m_failedImage)
	{
		std::cerr << "Failed to load image at " << imageName << std::endl;
		std::cerr << "Please check image exists and add any external paths with AddSearchPath()" << std::endl;
		return false;
	}

	//set transparency if required
	if(imageNode.attribute("trans"))
	{
		image.createMaskFromColor(m_ColourFromHex(imageNode.attribute("trans").as_string()));
	}

	//load image to texture
	sf::Texture texture;
	texture.loadFromImage(image);
	m_imageLayerTextures.push_back(texture);

	//add texture to layer as sprite, set layer properties
	MapTile tile;
	tile.sprite.setTexture(m_imageLayerTextures.back());
	MapLayer layer(ImageLayer);
	layer.name = imageLayerNode.attribute("name").as_string();
	if(imageLayerNode.attribute("opacity"))
	{
		layer.opacity = imageLayerNode.attribute("opacity").as_float();
		sf::Uint8 opacity = static_cast<sf::Uint8>(255.f * layer.opacity);
		tile.sprite.setColor(sf::Color(255u, 255u, 255u, opacity));
	}
	layer.tiles.push_back(tile);

	//parse layer properties
	if(pugi::xml_node propertiesNode = imageLayerNode.child("properties"))
		m_ParseLayerProperties(propertiesNode, layer);

	//push back layer
	m_layers.push_back(layer);

	return true;
}

void MapLoader::m_ParseLayerProperties(const pugi::xml_node& propertiesNode, MapLayer& layer)
{
	pugi::xml_node propertyNode = propertiesNode.child("property");
	while(propertyNode)
	{
		std::string name = propertyNode.attribute("name").as_string();
		std::string value = propertyNode.attribute("value").as_string();
		layer.properties[name] = value;
		propertyNode = propertyNode.next_sibling("property");
		std::cerr << "Added layer property " << name << " with value " << value << std::endl;
	}
}

void MapLoader::m_CreateDebugGrid()
{
	sf::Color debugColour(0u, 0u, 0u, 120u);
	float mapHeight = static_cast<float>(m_tileHeight * m_height);
	for(int x = 0; x <= m_width; x += 2)
	{
		float posX = static_cast<float>(x * (m_tileWidth / m_tileRatio));
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(posX, 0.f)), debugColour));
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(posX, mapHeight)), debugColour));
		posX += static_cast<float>(m_tileWidth) / m_tileRatio;
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(posX, mapHeight)), debugColour));
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(posX, 0.f)), debugColour));
		posX += static_cast<float>(m_tileWidth) / m_tileRatio;
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(posX, 0.f)), debugColour));
	}
	float mapWidth = static_cast<float>(m_tileWidth * (m_width / m_tileRatio));
	for(int y = 0; y <= m_height; y += 2)
	{
		float posY = static_cast<float>(y *m_tileHeight);
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(0.f, posY)), debugColour));
		posY += static_cast<float>(m_tileHeight);
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(0.f, posY)), debugColour));
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(mapWidth, posY)), debugColour));
		posY += static_cast<float>(m_tileHeight);
		m_gridVertices.append(sf::Vertex(IsometricToOrthogonal(sf::Vector2f(mapWidth, posY)), debugColour));
	}

	m_gridVertices.setPrimitiveType(sf::LinesStrip);

}

void MapLoader::m_SetIsometricCoords(MapLayer& layer)
{
	//float offset  = static_cast<float>(m_width * m_tileWidth) / 2.f;
	for(auto tile = layer.tiles.begin(); tile != layer.tiles.end(); ++tile)
	{
		sf::Int16 posX = (tile->gridCoord.x - tile->gridCoord.y) * (m_tileWidth / 2);
		sf::Int16 posY = (tile->gridCoord.y + tile->gridCoord.x) * (m_tileHeight / 2);
		tile->sprite.setPosition(static_cast<float>(posX)/* + offset*/, static_cast<float>(posY));
	}
}

void MapLoader::m_DrawLayer(sf::RenderTarget& rt, MapLayer& layer, bool debug)
{
	layer.Cull(m_bounds);
	rt.draw(layer);

	if(debug && layer.type == ObjectGroup)
	{
		for(const auto& object : layer.objects)		
			if(m_bounds.intersects(object.GetAABB()))
				object.DrawDebugShape(rt);
	}
}

std::string MapLoader::m_FileFromPath(const std::string& path)
{
	assert(!path.empty());

	for(auto it = path.rbegin(); it != path.rend(); ++it)
	{
		if(*it == '/' || *it == '\\')
		{
			int pos = std::distance(path.rbegin(), it);
			return path.substr(path.size() - pos);
		}
	}
	return path;
}

void MapLoader::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	sf::View view  = rt.getView();
	if(view.getCenter() != m_lastViewPos)
	{
		sf::FloatRect bounds;
		bounds.left = view.getCenter().x - (view.getSize().x / 2.f);
		bounds.top = view.getCenter().y - (view.getSize().y / 2.f);
		bounds.width = view.getSize().x;
		bounds.height = view.getSize().y;

		//add a tile border to prevent gaps appearing
		bounds.left -= static_cast<float>(m_tileWidth);
		bounds.top -= static_cast<float>(m_tileHeight);
		bounds.width += static_cast<float>(m_tileWidth * 2);
		bounds.height += static_cast<float>(m_tileHeight * 2);
		m_bounds = bounds;
	}
	m_lastViewPos = view.getCenter();

	for(auto& layer : m_layers)
		rt.draw(layer);
}

//decoding and utility functions
sf::Color MapLoader::m_ColourFromHex(const char* hexStr) const
{
	//TODO proper checking valid string length
	unsigned int value, r, g, b;
	std::stringstream input(hexStr);
	input >> std::hex >> value;

	r = (value >> 16) & 0xff;
	g = (value >> 8) & 0xff;
	b = value & 0xff;

	return sf::Color(r, g, b);
}

bool MapLoader::m_Decompress(const char* source, std::vector<unsigned char>& dest, int inSize, int expectedSize)
{
	if(!source)
	{
		std::cerr << "Input string is empty, decompression failed." << std::endl;
		return false;
	}

	int currentSize = expectedSize;
	//TODO switch to std::make_unique when compatible with all compilers
	std::unique_ptr<unsigned char[]> byteArray(new unsigned char[expectedSize / sizeof(unsigned char)]);
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	stream.next_in = (Bytef*)source;
	stream.avail_in = inSize;
	stream.next_out = (Bytef*)byteArray.get();
	stream.avail_out = expectedSize;

	if(inflateInit2(&stream, 15 + 32) != Z_OK)
	{
		std::cerr << "inflate 2 failed" << std::endl;
		return false;
	}

	int result = 0;
	do
	{
		result = inflate(&stream, Z_SYNC_FLUSH);

		switch(result)
		{
		case Z_NEED_DICT:
		case Z_STREAM_ERROR:
			result = Z_DATA_ERROR;
		case Z_DATA_ERROR:
		case Z_MEM_ERROR:
			inflateEnd(&stream);
			std::cerr << result << std::endl;
			return false;
		}

		if(result != Z_STREAM_END)
		{
			int oldSize = currentSize;
			currentSize *= 2;
			std::unique_ptr<unsigned char[]> newArray(new unsigned char[currentSize / sizeof(unsigned char)]);
			std::memcpy(newArray.get(), byteArray.get(), currentSize / 2);
			byteArray = std::move(newArray);
			
			stream.next_out = (Bytef*)(byteArray.get() + oldSize);
			stream.avail_out = oldSize;

		}
	}
	while(result != Z_STREAM_END);

	if(stream.avail_in != 0)
	{
		std::cerr << "stream.avail_in is 0" << std::endl;
		std::cerr << "zlib decompression failed." << std::endl;
		return false;
	}

	const int outSize = currentSize - stream.avail_out;
	inflateEnd(&stream);

	std::unique_ptr<unsigned char[]> newArray(new unsigned char[outSize / sizeof(unsigned char)]);
	std::memcpy(newArray.get(), byteArray.get(), outSize);
	byteArray = std::move(newArray);

	//copy bytes to vector
	int length = currentSize / sizeof(unsigned char);	
	dest.insert(dest.begin(), &byteArray[0], &byteArray[length]);

	return true;
}

sf::Image& MapLoader::m_LoadImage(const std::string& imageName)
{
	for(const auto& p : m_searchPaths)
	{
		const auto i = m_cachedImages.find(p + imageName);
		if(i != m_cachedImages.cend())
			return *i->second;
	}

	//else attempt to load
	std::shared_ptr<sf::Image> newImage = std::make_shared<sf::Image>();

	//try other paths first
	bool loaded = false;
	std::string path;
	for(const auto& p : m_searchPaths)
	{
		path = p + imageName;		
		loaded = newImage->loadFromFile(path);
		if(loaded) break;
	}
	if(!loaded)
	{
		newImage->create(20u, 20u, sf::Color::Yellow);
		m_failedImage = true;
		path = "default";
	}

	m_cachedImages[path] = newImage;
    return *m_cachedImages[path];
}



//base64 decode function taken from:
/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 Ren� Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   Ren� Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

namespace tmx
{
static const std::string base64_chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz"
				"0123456789+/";


static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

static std::string base64_decode(std::string const& encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4)
		{
			for (i = 0; i <4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret += char_array_3[j];
	}

	return ret;
}
};
