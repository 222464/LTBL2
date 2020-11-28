Let There Be Light 2
=======

A 2D dynamic soft shadows system with accurate penumbras/antumbras.

Install
-----------

LTBL2 relies only on SFML.

To get SFML, choose a package from here: [http://www.sfml-dev.org/download/sfml/2.2/](http://www.sfml-dev.org/download/sfml/2.2/)

LTBL2 uses CMake as the build system. You can get CMake here: [http://www.cmake.org/download/](http://www.cmake.org/download/)

Quick Start
-----------

The first step is to include LTBL2's light system:

```cpp
#include <ltbl/lighting/LightSystem.h>
```

To use LTBL2, you must first load the resources LTBL2 requires. The resources are located in the resources directory.

You will need to load 2 SFML shader objects:

```cpp
sf::Shader unshadowShader;
sf::Shader lightOverShapeShader;
unshadowShader.loadFromFile("resources/unshadowShader.vert", "resources/unshadowShader.frag");
lightOverShapeShader.loadFromFile("resources/lightOverShapeShader.vert", "resources/lightOverShapeShader.frag");
```

You will also need to load a texture:

```cpp
sf::Texture penumbraTexture;
penumbraTexture.loadFromFile("resources/penumbraTexture.png");
penumbraTexture.setSmooth(true);
```

It is important that you set the texture filtering to smooth, otherwise it will look pixelated.

Now you can create the LightSystem object:

```cpp
ltbl::LightSystem ls;
ls.create(sf::FloatRect(-1000.0f, -1000.0f, 1000.0f, 1000.0f), window->getSize(), penumbraTexture, unshadowShader, lightOverShapeShader);
```

Where the first parameter is a starting region for the quadtree (doesn't need to be exact, it will automatically adjust itself!).
The second parameter is the size of the rendering region. This is typically the size of the window you ultimately want to apply the lighting to.
The other parameters are the resources we loaded earlier.

LTBL2 has 2 basic light types: Point emission and direction emission.
Point emission can be used for point lights and spot lights. Direction emission is mostly for sunlight.

To create a light, you will need to create a light mask texture. Two defaults, one for point and one for direction, are provided in the resources directory.
The light mask texture defines the shape of the light source.

LTBL2 lights use SFML sprites to render. So you will set the light's sprite to use your mask texture, and then properly set the sprite's origin, size, position, and rotation as is usual with SFML.

Below is an example for creating one point light and one directional light:

```cpp
std::shared_ptr<ltbl::LightPointEmission> light = std::make_shared<ltbl::LightPointEmission>();

light->emissionSprite.setOrigin(<some_origin>);
light->emissionSprite.setTexture(<mask_texture>);
light->emissionSprite.setColor(<color>);
light->emissionSprite.setPosition(<position_of_light>);
light->localCastCenter = sf::Vector2f(0.0f, 0.0f); // This is where the shadows emanate from relative to the sprite

ls.addLight(light);

...

std::shared_ptr<ltbl::LightDirectionEmission> light = std::make_shared<ltbl::LightDirectionEmission>();

light->emissionSprite.setTexture(<mask_texture>);
light->castDirection = sf::Vector2f(<cast_direction>);

ls.addLight(light);
```

To create occluders, you must create a ltbl::LightShape object, and set the SFML shape it contains to represent the occluder:

```cpp
std::shared_ptr<ltbl::LightShape> lightShape = std::make_shared<ltbl::LightShape>();

lightShape->shape.setPointCount(<number_of_points>);

for (int j = 0; j < fixedPoints.size(); j++)
	lightShape->shape.setPoint(j, <point>);

lightShape->shape.setPosition(<position>);

ls.addShape(lightShape);
```

More instructions to come. You are of course welcome to post on the SFML forum thread for help: [http://en.sfml-dev.org/forums/index.php?topic=16895.0](http://en.sfml-dev.org/forums/index.php?topic=16895.0)

License
-----------

LTBL2
Copyright (C) 2014-2020 Eric Laukien

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

------------------------------------------------------------------------------

LTBL2 uses the following external libraries:

SFML - source code is licensed under the zlib/png license.
