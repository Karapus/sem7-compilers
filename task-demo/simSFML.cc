extern "C" {
#include "sim.h"
}
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <cstdlib>
#include <limits>

sf::RenderWindow Window{sf::VideoMode(IMG_X, IMG_Y), "SFML"};
sf::Image Field;
sf::Uint8 Pixels[IMG_X * IMG_Y * 4];

void simSetPixel(int X, int Y, struct SIM_Color Color) {
  auto Base = 4 * (Y * IMG_X + X);
  Pixels[Base + 0] = Color.R;
  Pixels[Base + 1] = Color.G;
  Pixels[Base + 2] = Color.B;
  Pixels[Base + 3] = Color.A;
}

void simFlush() {
  Field.create(IMG_X, IMG_Y, Pixels);
  sf::Texture Texture;
  Texture.loadFromImage(Field);
  sf::Sprite Sprite;
  Sprite.setTexture(Texture, true);
  Window.clear();
  Window.draw(Sprite);
  Window.display();
  sf::sleep(sf::seconds(1));
}

int simRand() { return std::rand(); }

int simIsRunning() {
  sf::Event Event;
  while (Window.pollEvent(Event)) {
    if (Event.type == sf::Event::Closed)
      Window.close();
  }
  return Window.isOpen();
}
