#include "VisualMusic.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/Sleep.hpp>

int main() {
	sf::RenderWindow window(
		sf::VideoMode(sf::Vector2u(640, 480)),
		"Test",
		sf::State::Windowed,
		sf::ContextSettings(0u, 0u, 4u)
	);
	//window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(60);

	sf::RectangleShape playingline;
	playingline.setFillColor(sf::Color::Red);
	playingline.setSize({ 1.0f, 1.0f });
	playingline.setOrigin({ 0.5f, 0.5f });
	playingline.setScale({ 1.0f, 1024.0f });

	sf::RenderStates states;
	states.transform.translate({ 320.5f, 240.5f });
	//states.transform.scale(1.0f, 1.5f);

	VisualMusic music;

	//music.openFromFile("../../The Fear.mp3");
	//music.openFromFile("../../test.mp3");
	music.openFromFile("D:\\音乐\\潘めぐみ,藤田咲,田辺留依 - シカ色デイズ.mp3");
	//music.openFromFile("D:\\ProgramData\\CloudMusic_Download\\Mitchie M,初音ミク - FREELY TOMORROW.flac");

	//music.setPlayingOffset(sf::seconds(220.0f));

	music.play();

	sf::Music m;

	bool isRunning = true;
	while (isRunning) {
		window.handleEvents(
			[&](const sf::Event::Closed&) { 
				isRunning = false;
			},
			[&](const sf::Event::KeyPressed& keyPress) {
				switch (keyPress.code) {
				case sf::Keyboard::Key::Space:
					if (music.getStatus() == sf::Music::Status::Paused)
						music.play();
					else
						music.pause();
					break;
				}
			}
		);

		window.clear(sf::Color::Black);

		window.draw(playingline, states);
		window.draw(music, states);

		window.display();
	}

	window.close();

	return 0;
}