#pragma once

#include <SDL2/SDL.h>

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

    StickFigure();
    ~StickFigure() = default;

    void handle_events(SDL_Event const &event);
    void update(double delta_time);
    void draw(SDL_Surface *window_surface);
 private:
    Direction    m_direction;

    Spritesheet  m_spritesheet;
    SDL_Rect     m_position;

    double       m_x;
    double       m_y;
};
