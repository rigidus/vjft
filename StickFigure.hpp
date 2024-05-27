// StickFigure.hpp
#pragma once

#include <SDL2/SDL.h>
#include "SpriteSheet.hpp"

class StickFigure
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

    StickFigure(SDL_Renderer* renderer, const char* spritePath, int startX, int startY);
    ~StickFigure() = default;

    void update(double delta_time);
    void draw(SDL_Renderer *renderer);

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void setDirection(Direction newDirection);

private:
    Spritesheet  spriteSheet;
    int          spriteSheetColumn;
    Direction    direction;

    SDL_Rect     position;
    double       m_x;
    double       m_y;
};
