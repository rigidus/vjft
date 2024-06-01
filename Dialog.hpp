// Dialog.hpp
#pragma once
#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <functional>
// #include "TextRenderer.hpp"
#include "TextField.hpp"

class Dialog {
public:
    Dialog(SDL_Renderer* renderer, TTF_Font* font);
    void show();
    void hide();
    bool isActive() const;
    void handleEvent(SDL_Event& event);
    void render(SDL_Renderer* renderer);
    void setCloseCallback(std::function<void()> callback);
private:
    bool active;
    SDL_Rect dialogRect;
    // std::string inputText;
    SDL_Color backgroundColor;
    SDL_Color textColor;
    // TextRenderer textRenderer;
    TextField textField;
    std::function<void()> closeCallback;
};
