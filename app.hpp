#pragma once

#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <memory>

#include "stick_figure.hpp"
#include "utilities.hpp"

class App
{
public:
    App();
    ~App();

    void loop();
    void update(double delta_time);
    void draw();
private:
    StickFigure   m_stick_figure;

    SDL_Window*   m_window;
    SDL_Surface*  m_window_surface;
    SDL_Renderer* m_renderer;
    TTF_Font*     m_font;
    SDL_Texture*  m_texture;
    SDL_Texture*  m_sprite_texture;
    SDL_Event     m_window_event;
    SDL_Rect      dstrect;
    SDL_Rect      m_sprite_srcRect;
    SDL_Rect      m_sprite_dstRect;
};
