#pragma once

#include <SDL2/SDL.h>
#include <iostream>

#include "stick_figure.hpp"
#include "utilities.hpp"

SDL_Surface* load_surface(const char* path);

class Application
{
public:
    Application();
    ~Application();

    void loop();
    void update(double delta_time);
    void draw();
private:
    StickFigure m_stick_figure;

    SDL_Window  *m_window;
    SDL_Surface *m_window_surface;
    SDL_Event    m_window_event;
};
