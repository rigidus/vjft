// Viewport.cpp

#include "Viewport.hpp"

Viewport::Viewport(int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height) {}

void Viewport::setPosition(int newX, int newY)
{
    x = newX;
    y = newY;
}

int Viewport::getX()  { return x; }
int Viewport::getY()  { return y; }
int Viewport::getWidth()  { return width; }
int Viewport::getHeight() { return height; }

bool Viewport::isVisible(const SDL_Rect& objectRect)
{
    return (objectRect.x + objectRect.w > this->getX() &&
            objectRect.x < this->getX() + this->getWidth() &&
            objectRect.y + objectRect.h > this->getY() &&
            objectRect.y < this->getY() + this->getHeight());
}
