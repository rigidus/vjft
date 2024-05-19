#pragma once

#include <SDL2/SDL.h>
#include <iostream>

SDL_Surface* load_surface(const char* path);

class Application
{
public:
    Application();
    ~Application();

    void draw();
    void loop();
    void update(double delta_time);
private:
    SDL_Surface *m_image;
    SDL_Rect     m_image_position;

    double       m_image_x;
    double       m_image_y;

    SDL_Window  *m_window;
    SDL_Surface *m_window_surface;
    SDL_Event    m_window_event;
};
