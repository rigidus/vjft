// App.hpp
#pragma once

#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <memory>
#include <optional>
#include "StickFigure.hpp"
#include "TextRenderer.hpp"
#include "EventManager.hpp"
#include "Player.hpp"
#include "StackCleanup.hpp"

class App
{
public:
    App();
    ~App();

    void Cleanup();
    bool initSDL();
    bool initIMG();
    bool initTTF();
    bool initWindow();
    bool initRenderer();
    void processEvents();
    void handleKeyPress(SDL_Keycode key);
    void loop();
    void update(double delta_time);
    void draw();
private:
    bool          m_running;
    SDL_Window*   m_window;
    SDL_Renderer* m_renderer;
    std::optional<StickFigure> m_stick_figure;
    std::optional<Player> m_player;
    std::optional<TextRenderer> m_text_renderer;
    SDL_Texture*  m_text_texture;
    SDL_Rect      m_text_rect;
    EventManager  m_eventManager;
    StackCleanup  m_cleanupStack;
};
