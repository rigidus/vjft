// App.hpp
#pragma once

#include <SDL2/SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include <memory>
#include <optional>
#include "Figure.hpp"
#include "TextRenderer.hpp"
#include "EventManager.hpp"
#include "Player.hpp"
#include "StackCleanup.hpp"
#include "Scene.hpp"
#include "Viewport.hpp"

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
    void handleKeyRelease(SDL_Keycode key);
    void loop();
    void update(double delta_time);
    bool isVisible(const SDL_Rect& objectRect, std::shared_ptr<Viewport> viewport);
    void draw();

private:
    bool                         m_running;
    SDL_Window*                  m_window;
    SDL_Renderer*                m_renderer;
    std::shared_ptr<Figure>      m_figure1;
    std::shared_ptr<Figure>      m_figure2;
    std::shared_ptr<Player>      m_player1;
    std::shared_ptr<Player>      m_player2;
    std::optional<TextRenderer>  m_text_renderer;
    SDL_Texture*                 m_text_texture;
    SDL_Rect                     m_text_rect;
    EventManager                 m_eventManager;
    StackCleanup                 m_cleanupStack;
    Scene                        m_scene;
    std::shared_ptr<Viewport>    m_viewport;
};
