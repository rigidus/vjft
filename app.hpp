// app.hpp
#pragma once

#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <memory>
#include <optional>
#include "stick_figure.hpp"
#include "utilities.hpp"
#include "TextRenderer.hpp"
#include "EventManager.hpp"
#include "Player.hpp"

class App
{
public:
    App();
    ~App();

    void loop();
    void update(double delta_time);
    void draw();
private:
    bool          m_running;
    SDL_Window*   m_window;
    SDL_Surface*  m_window_surface;
    SDL_Renderer* m_renderer;
    TTF_Font*     m_font;
    SDL_Texture*  m_texture;
    SDL_Texture*  m_sprite_texture;;
    SDL_Rect      dstrect;
    SDL_Rect      m_sprite_srcRect;
    SDL_Rect      m_sprite_dstRect;
    TextRenderer* m_text_renderer;
    SDL_Texture*  m_text_texture;
    SDL_Rect      m_text_rect;

    EventManager m_eventManager;

    std::optional<StickFigure> m_stick_figure;
    std::optional<Player> m_player;
};
