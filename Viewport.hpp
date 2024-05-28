// Viewport.hpp
#pragma once

#include <SDL2/SDL.h>

class Viewport {
public:
    Viewport(int x, int y, int width, int height);
    void setPosition(int newX, int newY);
    void moveLeft(int amount);
    void moveRight(int amount);
    void moveUp(int amount);
    void moveDown(int amount);
    int getX();
    int getY();
    int getWidth();
    int getHeight();
    bool isVisible(const SDL_Rect& rect);

private:
    int x, y;
    int width, height;
};
