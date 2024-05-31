#include "Dialog.hpp"

Dialog::Dialog(SDL_Renderer* renderer, TTF_Font* font)
    : active(false),
      inputText(""),
      backgroundColor({0, 0, 0, 255}),
      textColor({255, 255, 255, 255}),
      textRenderer(renderer, font) {
    dialogRect = {200, 200, 400, 200}; // Example position and size
}


void Dialog::show() {
    active = true;
}


void Dialog::hide() {
    active = false;
    if (closeCallback) {
        closeCallback();
    }
}


bool Dialog::isActive() const {
    return active;
}


void Dialog::handleEvent(const SDL_Event& event) {
    if (!active) return;

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE) {
            hide();
        } else if (event.key.keysym.sym == SDLK_BACKSPACE && !inputText.empty()) {
            std::cerr << inputText << std::endl;
            inputText.pop_back();
        }
    } else if (event.type == SDL_TEXTINPUT) {
        inputText += event.text.text;
    }
}


void Dialog::render(SDL_Renderer* renderer) {
    if (!active) return;

    // Render dialog background
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &dialogRect);

    // Render input text
    int width, height;
    SDL_Texture* textTexture = textRenderer.renderText(inputText, textColor, width, height);
    if (textTexture) {
        SDL_Rect textRect = {dialogRect.x + 10, dialogRect.y + 10, width, height};
        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
        SDL_DestroyTexture(textTexture);
    }
}

void Dialog::setCloseCallback(std::function<void()> callback) {
    closeCallback = callback;
}
