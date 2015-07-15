#include <SFML/Graphics.hpp>

#include <ltbl/lighting/LightSystem.h>

#include <tmx/MapLoader.h>

#include <random>

int main() {
	std::mt19937 generator(time(nullptr));

	sf::RenderWindow window;

	sf::ContextSettings contextSettings;

	window.create(sf::VideoMode(1280, 720), "LTBL2", sf::Style::Default, contextSettings);

	sf::Texture pointLightTexture;

	pointLightTexture.loadFromFile("resources/directionLightTexture.png");

	pointLightTexture.setSmooth(true);

	sf::Texture penumbraTexture;

	penumbraTexture.loadFromFile("resources/penumbraTexture.png");

	penumbraTexture.setSmooth(true);

	ltbl::LightSystem ls;

	sf::Shader unshadowShader;

	unshadowShader.loadFromFile("resources/unshadowShader.vert", "resources/unshadowShader.frag");

	ls.create(ltbl::rectFromBounds(sf::Vector2f(-1000.0f, -1000.0f), sf::Vector2f(1000.0f, 1000.0f)), window.getSize(), penumbraTexture, unshadowShader);

	std::shared_ptr<ltbl::LightDirectionEmission> light = std::make_shared<ltbl::LightDirectionEmission>();

	light->_emissionSprite.setOrigin(sf::Vector2f(pointLightTexture.getSize().x * 0.5f, pointLightTexture.getSize().y * 0.5f));
	light->_emissionSprite.setTexture(pointLightTexture);
	light->_emissionSprite.setScale(sf::Vector2f(6.0f, 6.0f));
	light->_emissionSprite.setColor(sf::Color(255, 230, 200));
	light->_emissionSprite.setPosition(sf::Vector2f(0.0f, 0.0f));
	light->_castDirection = ltbl::vectorNormalize(sf::Vector2f(-0.1f, 0.6f));
	
	ls.addLight(light);

	tmx::MapLoader ml("resources/maps");

	ml.Load("testMap3.tmx");

	bool quit = false;

	sf::Clock clock;

	float dt = 0.017f;

	window.setVerticalSyncEnabled(true);

	for (int l = 0; l < ml.GetLayers().size(); l++)
	if (ml.GetLayers()[l].name == "collision") {
		for (int i = 0; i < ml.GetLayers()[l].objects.size(); i++) {
			tmx::MapObject &object = ml.GetLayers()[l].objects[i];

			std::list<sf::Vector2f> points;

			sf::Vector2f center(0.0f, 0.0f);

			for (int j = 0; j < object.PolyPoints().size(); j++) {
				sf::Vector2f point = sf::Vector2f(object.PolyPoints()[j].x, object.PolyPoints()[j].y);

				points.push_back(point);

				center += point;
			}

			center /= static_cast<float>(object.PolyPoints().size());

			// Fix winding
			sf::Vector2f lastPoint = points.front();
			points.pop_front();

			std::vector<sf::Vector2f> fixedPoints;

			fixedPoints.push_back(lastPoint);

			while (fixedPoints.size() < object.PolyPoints().size()) {
				sf::Vector2f centerToLastPoint = lastPoint - center;
				sf::Vector2f lastPointDirection = ltbl::vectorNormalize(sf::Vector2f(-centerToLastPoint.y, centerToLastPoint.x));

				float maxD = -999999.0f;

				std::list<sf::Vector2f>::iterator nextPointIt;

				// Get next point
				for (std::list<sf::Vector2f>::iterator it = points.begin(); it != points.end(); it++) {
					sf::Vector2f toPointNormalized = ltbl::vectorNormalize(*it - lastPoint);
					
					float d = ltbl::vectorDot(toPointNormalized, lastPointDirection);

					if (d > maxD) {
						maxD = d;
						nextPointIt = it;
					}
				}

				fixedPoints.push_back(*nextPointIt);

				points.erase(nextPointIt);
			}

			std::shared_ptr<ltbl::LightShape> lightShape = std::make_shared<ltbl::LightShape>();

			lightShape->_shape.setPointCount(object.PolyPoints().size());

			for (int j = 0; j < fixedPoints.size(); j++)
				lightShape->_shape.setPoint(j, fixedPoints[j]);

			lightShape->_shape.setPosition(object.GetPosition());

			ls.addShape(lightShape);
		}
	}

	sf::View view = window.getDefaultView();

	view.setCenter(sf::Vector2f(ml.GetMapSize().x * 0.5f, ml.GetMapSize().y * 0.5f));

	view.zoom(0.5f);

	do {
		clock.restart();

		// ----------------------------- Input -----------------------------

		sf::Event windowEvent;

		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				quit = true;
				break;
			}
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
			quit = true;

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			view.move(-2.0f, 0.0f);
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			view.move(2.0f, 0.0f);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			view.move(0.0f, 2.0f);
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			view.move(0.0f, -2.0f);

		window.setView(view);

		// ---------------------------- Rendering ----------------------------

		window.clear(sf::Color::White);

		ls.render(view, unshadowShader);

		sf::Sprite sprite;
		sprite.setTexture(ls.getLightingTexture());

		sf::RenderStates lightRenderStates;
		lightRenderStates.blendMode = sf::BlendMultiply;

		window.setView(window.getDefaultView());
		window.draw(sprite, lightRenderStates);
		window.setView(view);

		window.draw(ml);

		// -------------------------------------------------------------------

		window.display();

		dt = clock.getElapsedTime().asSeconds();
	} while (!quit);

	return 0;
}