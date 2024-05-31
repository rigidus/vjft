// Dialog.hpp
#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <functional>
#include "TextRenderer.hpp"

class Dialog {
public:
    Dialog(SDL_Renderer* renderer, TTF_Font* font);
    void show();
    void hide();
    bool isActive() const;
    void handleEvent(const SDL_Event& event);
    void render(SDL_Renderer* renderer);
    void setCloseCallback(std::function<void()> callback);
private:
    bool active;
    std::string inputText;
    SDL_Rect dialogRect;
    SDL_Color backgroundColor;
    SDL_Color textColor;
    TextRenderer textRenderer;
    std::function<void()> closeCallback;
};
