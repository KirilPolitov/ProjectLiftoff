#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <cmath>
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

static std::atomic<bool> stopSound{ false };
static std::atomic<bool> thrusterOn{ false };

static void ambientSoundThread() {
    sf::Music ambientSound;
    if (!ambientSound.openFromFile("Tranquility.ogg")) {
        return;
    }
    ambientSound.setLooping(true);
    ambientSound.setVolume(50.f);
    ambientSound.play();

    // Keep the thread alive
    while (!stopSound.load() && ambientSound.getStatus() == sf::SoundSource::Status::Playing) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

static void thrusterSound() {
    sf::Music engine;
    if (!engine.openFromFile("rocketEngine.ogg")) {
        return;
    }
	engine.setLooping(true);
    engine.setVolume(100.f);
    engine.play();
    // Keep the thread alive
    while (!stopSound.load() && engine.getStatus() == sf::SoundSource::Status::Playing) {
        if (!thrusterOn.load()) {
            engine.setVolume(0.f);
        }
        else engine.setVolume(100.f);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}


int main() {
    // Use the desktop resolution so the video mode matches the screen size
    sf::VideoMode videoMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(videoMode, "Project Lifoff");
	window.setFramerateLimit(165);
    window.setVerticalSyncEnabled(true);
	sf::Image icon;
    if (icon.loadFromFile("Cartoon_space_rocket.png")) { window.setIcon(icon); }
    sf::RectangleShape rectangle(sf::Vector2f(100.f, 100.f));
    rectangle.setOrigin(rectangle.getGeometricCenter());
    rectangle.setPosition({ 10.f, 10.f });
	sf::Texture rocketTexture;
	if (rocketTexture.loadFromFile("rocketSprite.png")) {
		rectangle.setTexture(&rocketTexture);
	}
    sf::CircleShape circle(600.f);
    circle.setOrigin(circle.getGeometricCenter());
    circle.setPosition({ 400.f, 300.f });
	sf::Texture planetTexture;
	if (planetTexture.loadFromFile("earthSprite.png")) {
		circle.setTexture(&planetTexture);
	}
	sf::CircleShape atmosphere(900.f);
	atmosphere.setOrigin(atmosphere.getGeometricCenter());
	atmosphere.setPosition({ 400.f, 300.f });
	sf::Texture atmosphereTexture;
    if (atmosphereTexture.loadFromFile("atmosphereOutline.png")) {
		atmosphere.setTexture(&atmosphereTexture);
    }
    sf::CircleShape moon(200.f);
	moon.setFillColor(sf::Color::White);
    moon.setOrigin(moon.getGeometricCenter());
    moon.setPosition({ 400.f, 5000.f });
	sf::Texture moonTexture;
	if (moonTexture.loadFromFile("moonSprite.png")) {
		moon.setTexture(&moonTexture);
	}
    sf::Clock clock;
    sf::Vector2f velocity(0.f, 0.f);
    sf::Vector2f velocityMoon(0.f, 0.f);
    float gravity = 50000000.f;
	float moonGravity = 10000000.f;
    float thrust = 200.f;
    float rotationSpeed = 3000.f;
    sf::View camera(window.getDefaultView());
	sf::Vector2f initialMoonDirection = sf::Vector2f(1.f, 0.f);
	velocityMoon = initialMoonDirection.normalized() * 100.f;

	std::thread soundThread(ambientSoundThread);
	std::thread engineThread(thrusterSound);

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
		//camera
        camera.setCenter(rectangle.getPosition());
        window.setView(camera);

        //moon orbit
        sf::Vector2f directionMoonPlanet = circle.getPosition() - moon.getPosition();
        float distanceMoonPlanet = directionMoonPlanet.length();
        sf::Vector2f nDMoonPlanet = directionMoonPlanet.normalized();
		velocityMoon += nDMoonPlanet * (gravity / (distanceMoonPlanet * distanceMoonPlanet)) * deltaTime;

        //gravity
		sf::Vector2f direction = circle.getPosition() - rectangle.getPosition();
		float distance = direction.length();
		sf::Vector2f normalizedDirection = direction.normalized();
		velocity += normalizedDirection * (gravity/ (distance * distance)) * deltaTime;

		sf::Vector2f directionToMoon = moon.getPosition() - rectangle.getPosition();
		float distanceToMoon = directionToMoon.length();
		sf::Vector2f normalizedDirectionToMoon = directionToMoon.normalized();
		velocity += normalizedDirectionToMoon * (moonGravity / (distanceToMoon * distanceToMoon)) * deltaTime;

        //collision
		float minDistance = rectangle.getSize().length() / 5.8f + circle.getRadius();
        if (distance < minDistance) {
            sf::Vector2f overlap = normalizedDirection * (minDistance - distance);
            rectangle.move(-overlap);
			velocity = sf::Vector2f(0.f, 0.f);
        }

		float minDistanceToMoon = rectangle.getSize().length() / 3.f + moon.getRadius();
        if (distanceToMoon < minDistanceToMoon) {
            sf::Vector2f overlap = normalizedDirectionToMoon * (minDistanceToMoon - distanceToMoon);
			rectangle.move(-overlap);
            velocity = velocityMoon;
		}
		
		//atmosphere drag
        float atmosphereDistance = circle.getRadius() * 1.5f;
        if (distance < atmosphereDistance) {
			float pressure = 1.f * std::exp(-(distance - minDistance) / ((atmosphereDistance - minDistance)/3));
			float stress = pressure * velocity.length() / 2.f;
            if (stress > 100.f) {
				rectangle.setFillColor(sf::Color::Red);
            }
			else rectangle.setFillColor(sf::Color::White);
			velocity -= velocity * pressure * deltaTime;
		}
		else rectangle.setFillColor(sf::Color::White);

        while (auto opt = window.pollEvent()) {
            const sf::Event& event = *opt;
            if (event.is<sf::Event::Closed>()) {
				stopSound.store(true);
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
				soundThread.join();
				engineThread.join();
                window.close();
            }
        }
		//controls
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            rectangle.rotate(sf::degrees(0.1f * rotationSpeed * deltaTime));
		}
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            rectangle.rotate(sf::degrees(-0.1f * rotationSpeed * deltaTime));
        }
		float angleRad = rectangle.getRotation().asRadians();
		sf::Vector2f forwardVector(std::cos(angleRad), std::sin(angleRad));
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
			velocity += forwardVector * thrust * deltaTime;
			thrusterOn.store(true);
        }
		else thrusterOn.store(false);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
            velocity -= forwardVector * thrust * deltaTime;
		}
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)) {
            camera.zoom(std::pow(0.1f, deltaTime));
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
            camera.zoom(std::pow(10.f, deltaTime));
		}
		//update position
		rectangle.move(velocity * deltaTime);
		moon.move(velocityMoon * deltaTime);
		//rendering
        window.clear(sf::Color::Black);
        window.draw(atmosphere);
		window.draw(rectangle);
        window.draw(moon);
		window.draw(circle);
        window.display();
    }
    return 0;
}