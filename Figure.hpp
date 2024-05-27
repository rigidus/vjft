// Figure.hpp
#pragma once

#include <SDL2/SDL.h>
#include "SpriteSheet.hpp"

class Figure
{
public:
    enum class Direction
    {
        NONE,
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    Figure(SDL_Renderer* renderer, const char* spritePath, int startX, int startY);
    ~Figure() = default;

    void update(double delta_time);
    void draw(SDL_Renderer *renderer);

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void setDirection(Direction newDirection);

    SDL_Rect getBoundingBox() const;

private:
    Spritesheet  spriteSheet;
    int          spriteSheetColumn;
    Direction    direction;

    SDL_Rect     position;
    double       m_x;
    double       m_y;
};
