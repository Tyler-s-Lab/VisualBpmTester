#include "VisualMusic.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/Sleep.hpp>

int main() {
	sf::RenderWindow window(
		sf::VideoMode(640, 480),
		"Test",
		sf::Style::Titlebar | sf::Style::Close,
		sf::ContextSettings(0u, 0u, 4u)
	);
	window.setVerticalSyncEnabled(true);

	sf::RectangleShape playingline;
	playingline.setFillColor(sf::Color::Red);
	playingline.setSize(sf::Vector2f(1.0f, 1.0f));
	playingline.setOrigin(0.5f, 0.5f);
	playingline.setScale(1.0f, 512.0f);

	sf::RenderStates states;
	states.transform.translate(320.5f, 240.5f);
	states.transform.scale(1.0f, 2.0f);

	VisualMusic music;

	music.openFromFile("../../The Fear.mp3");
	//music.openFromFile("../../test.mp3");
	//music.openFromFile("D:\\ProgramData\\CloudMusic_Download\\潘めぐみ,藤田咲,田辺留依 - シカ色デイズ.mp3");
	//music.openFromFile("D:\\ProgramData\\CloudMusic_Download\\Mitchie M,初音ミク - FREELY TOMORROW.flac");

	music.play();

	bool isRunning = true;
	sf::Event evt;
	while (isRunning) {
		while (window.pollEvent(evt)) {
			switch (evt.type) {
			case sf::Event::Closed:
				isRunning = false;
				break;
			case sf::Event::KeyReleased:
				switch (evt.key.code) {
				case sf::Keyboard::Space:
					if (music.getStatus() == music.Paused)
						music.play();
					else
						music.pause();
					break;
				}
				break;
			}
		}

		window.clear(sf::Color::Black);

		window.draw(playingline, states);
		window.draw(music, states);

		window.display();
	}

	window.close();

	return 0;
}