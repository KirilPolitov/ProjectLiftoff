#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Window.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <iostream>
#include <cmath>
#include <string>
#include <ctime>
#include <cstdlib>
#ifdef _WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

static std::atomic<bool> stopSound{ false };
static std::atomic<bool> thrusterOn{ false };
static std::atomic<bool> dead{ false };
static std::atomic<float> musicVolume{ 50.f };
static std::atomic<float> engineMaxVol{ 100.f };

static void ambientSoundThread() {
    sf::Music ambientSound;
    if (!ambientSound.openFromFile("Tranquility.ogg")) {
        return;
    }
    ambientSound.setLooping(true);
    ambientSound.setVolume(musicVolume);
    ambientSound.play();

    // Keep the thread alive
    while (!stopSound.load() && ambientSound.getStatus() == sf::SoundSource::Status::Playing) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        ambientSound.setVolume(musicVolume);
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
        else engine.setVolume(engineMaxVol);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}


int main() {
    srand(time(0));
    // Use the desktop resolution so the video mode matches the screen size
    sf::VideoMode videoMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(videoMode, "Project Lifoff");
	window.setFramerateLimit(165);
    window.setVerticalSyncEnabled(true);
	sf::Image icon;
    if (icon.loadFromFile("Cartoon_space_rocket.png")) { window.setIcon(icon); }
	sf::RenderTexture userInterface({ videoMode.size.x, videoMode.size.y });
	userInterface.setView(window.getDefaultView());
    float uiScale = window.getSize().y / 1080.f;
    float uiScaleX = window.getSize().x / 1920.f;
    sf::Font font("spaceAge.ttf");
    sf::Text velText(font);
	velText.setCharacterSize(30 * uiScale);
	velText.setFillColor(sf::Color::White);
	velText.setPosition({ 10.f, 10.f });
	sf::Text atmoStressText(font);
	atmoStressText.setCharacterSize(30 * uiScale);
	atmoStressText.setFillColor(sf::Color::White);
	atmoStressText.setPosition({ 10.f, 60.f });
    sf::Text planetRelVel(font);
    planetRelVel.setCharacterSize(30 * uiScale);
    planetRelVel.setFillColor(sf::Color::White);
    planetRelVel.setPosition({ 10.f, 110.f });
    sf::Text moonRelVel(font);
    moonRelVel.setCharacterSize(30 * uiScale);
    moonRelVel.setFillColor(sf::Color::White);
    moonRelVel.setPosition({ 10.f, 160.f });
    sf::Text deathMessage(font);
    deathMessage.setCharacterSize(200 * uiScale);
    deathMessage.setFillColor(sf::Color::Red);
    deathMessage.setString("YOU DIED");
    deathMessage.setOrigin(deathMessage.getLocalBounds().getCenter());
    deathMessage.setPosition({ (window.getSize().x / 2.f), (window.getSize().y / 2.f) });
    std::string collisionDeaths[3] = {"You hit the ground too hard!", "You are bad at falling!", "SPLAT!!!"};
    std::string aeroDeaths[3] = {"You sure like the wind!", "SLOW DOWN!", "Is it getting hot in here?"};
    std::string cosmicRadDeath = "Cosmic radiation is enough to break the strongest wills.....";
    sf::Text deathComment(font);
    deathComment.setCharacterSize(30 * uiScale);
    deathComment.setFillColor(sf::Color::White);
    sf::Text cosmicRadWarn(font);
    cosmicRadWarn.setCharacterSize(30 * uiScale);
    cosmicRadWarn.setString("WARNING\nCosmic radiation increasing exponentially\nturn back now or face\nirreversible harm or even death!");
    cosmicRadWarn.setPosition({10, (window.getSize().y / 1.5f)});
    sf::Text respawnRemind(font);
    respawnRemind.setCharacterSize(30 * uiScale);
    respawnRemind.setFillColor(sf::Color::Cyan);
    respawnRemind.setString("Press Escape to respawn");
    respawnRemind.setOrigin(respawnRemind.getLocalBounds().getCenter());
    respawnRemind.setPosition({ (window.getSize().x / 2.f), (window.getSize().y / 1.2f) });
    sf::Text collisonFatality(font);
    collisonFatality.setCharacterSize(30 * uiScale);
    collisonFatality.setFillColor(sf::Color::White);
    collisonFatality.setString("Fatal collison Speed: 250");
    collisonFatality.setOrigin(collisonFatality.getLocalBounds().getCenter());
    collisonFatality.setPosition({ (1580.f * uiScaleX), 30.f });
    sf::Text aeroFatality(font);
    aeroFatality.setCharacterSize(30 * uiScale);
    aeroFatality.setFillColor(sf::Color::White);
    aeroFatality.setString("Fatal aerodynamic stress: 150");
    aeroFatality.setOrigin(aeroFatality.getLocalBounds().getCenter());
    aeroFatality.setPosition({ (1533.f * uiScaleX), 80.f });
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
    sf::View cameraUI;
    cameraUI.setSize(window.getView().getSize());
    cameraUI.setCenter(window.getView().getCenter());
    userInterface.setView(cameraUI);
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

        short index = rand() % 3;
        //collision
		float minDistance = rectangle.getSize().length() / 5.8f + circle.getRadius();
		if (distance < minDistance && velocity.length() > 250.f) {
            dead.store(true);
            deathComment.setString(collisionDeaths[index]);
        }
        if (distance < minDistance) {
            sf::Vector2f overlap = normalizedDirection * (minDistance - distance);
            rectangle.move(-overlap);
			velocity = sf::Vector2f(0.f, 0.f);
        }

		float minDistanceToMoon = rectangle.getSize().length() / 3.f + moon.getRadius();
		float moonRelativeVelocity = (velocity - velocityMoon).length();
        if (distanceToMoon < minDistanceToMoon && moonRelativeVelocity > 250.f) {
            dead.store(true);
            deathComment.setString(collisionDeaths[index]);
        }
        if (distanceToMoon < minDistanceToMoon) {
            sf::Vector2f overlap = normalizedDirectionToMoon * (minDistanceToMoon - distanceToMoon);
			rectangle.move(-overlap);
            velocity = velocityMoon;
		}
		
		//atmosphere drag
        float atmosphereDistance = circle.getRadius() * 1.5f;
        float pressure = 1.f * std::exp(-(distance - minDistance) / ((atmosphereDistance - minDistance) / 3));
        float stress = pressure * velocity.length() / 2.f;
        if (distance < atmosphereDistance) {
            if (stress > 150.f) {
				dead.store(true);
                deathComment.setString(aeroDeaths[index]);
			}
            if (stress > 100.f) {
				rectangle.setFillColor(sf::Color::Red);
            }
			else rectangle.setFillColor(sf::Color::White);
			velocity -= velocity * pressure * deltaTime;
		}
		else rectangle.setFillColor(sf::Color::White);

        //UI elements
        velText.setString("Velocity: " + std::to_string(static_cast<int>(floor(velocity.length()))));
        atmoStressText.setString("Atmosphereic Stress: " + std::to_string(static_cast<int>(floor(stress))));
        planetRelVel.setString("Planet Relative Velocity: " + std::to_string(static_cast<int>(floor(velocity.length()))));
        moonRelVel.setString("Moon Relative Velocity: " + std::to_string(static_cast<int>(floor(moonRelativeVelocity))));
        deathComment.setOrigin(deathComment.getLocalBounds().getCenter());
        deathComment.setPosition({ (window.getSize().x / 2.f), (window.getSize().y / 1.5f) });
        if (distance > 50 * circle.getRadius()) {
            dead.store(true);
            deathComment.setString(cosmicRadDeath);
        }
        else if (distance > 35 * circle.getRadius()) {
            cosmicRadWarn.setFillColor(sf::Color::Red);
        }
        else if (distance > 20 * circle.getRadius()) {
            cosmicRadWarn.setFillColor(sf::Color::Yellow);
        }

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
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) && !dead.load() == true) {
			velocity += forwardVector * thrust * deltaTime;
			thrusterOn.store(true);
        }
		else thrusterOn.store(false);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) && !dead.load() == true) {
            velocity -= forwardVector * thrust * deltaTime;
		}
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z)) {
            camera.zoom(std::pow(0.1f, deltaTime));
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::X)) {
            camera.zoom(std::pow(10.f, deltaTime));
		}
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape) && dead.load() == true) {
			dead.store(false);
			rectangle.setPosition({ 10.f, 10.f });
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::I)) {
            if (musicVolume.load() < 100.f)
                musicVolume.store(musicVolume.load() + 100.f * deltaTime);
            if (musicVolume.load() > 100.f) musicVolume.store(100.f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K)) {
            if (musicVolume.load() > 0.f)
                musicVolume.store(musicVolume.load() - 100.f * deltaTime);
            if (musicVolume.load() < 0.f) musicVolume.store(0.f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::O)) {
            if (engineMaxVol.load() < 100.f)
                engineMaxVol.store(engineMaxVol.load() + 100.f * deltaTime);
            if (engineMaxVol.load() > 100.f) engineMaxVol.store(100.f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L)) {
            if (engineMaxVol.load() > 0.f)
                engineMaxVol.store(engineMaxVol.load() - 100.f * deltaTime);
            if (engineMaxVol.load() < 0.f) engineMaxVol.store(0.f);
        }
		//update position
		if (dead.load() == true)
            velocity = sf::Vector2f(0.f, 0.f);
		rectangle.move(velocity * deltaTime);
		moon.move(velocityMoon * deltaTime);
		//rendering
        window.clear(sf::Color::Black);
		userInterface.clear(sf::Color::Transparent);
        window.draw(atmosphere);
		if (!dead.load() == true)
		    window.draw(rectangle);
        window.draw(moon);
		window.draw(circle);
		window.setView(cameraUI);
		userInterface.draw(velText);
        if (distance < atmosphereDistance) {
            userInterface.draw(atmoStressText);
            userInterface.draw(aeroFatality);
        }
        if (distance < 1500.f)
            userInterface.draw(planetRelVel);
        if (distanceToMoon < 700.f)
            userInterface.draw(moonRelVel);
        if (distance < 1500.f || distanceToMoon < 700.f)
            userInterface.draw(collisonFatality);
        if (distance > 20 * circle.getRadius() && dead.load() != true)
            userInterface.draw(cosmicRadWarn);
        if (dead.load() == true) {
            userInterface.draw(deathMessage);
            userInterface.draw(deathComment);
            userInterface.draw(respawnRemind);
        }
		sf::Sprite uiSprite(userInterface.getTexture());
		uiSprite.setPosition(cameraUI.getCenter() - cameraUI.getSize() / 2.f);
		window.draw(uiSprite);
        window.display();
        userInterface.display();
    }
    return 0;
}