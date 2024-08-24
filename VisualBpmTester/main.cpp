#include "VisualMusic.h"
#include <SFML/System/Sleep.hpp>

int main() {
	VisualMusic music;
	//sf::Music music;

	music.openFromFile("../../The Fear.mp3");

	music.play();

	while (music.getStatus() == music.Playing) {
		sf::sleep(sf::milliseconds(30));
	}

	return 0;
}