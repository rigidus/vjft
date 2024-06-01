// Dialog.cpp

#include "Dialog.hpp"

// Dialog::Dialog(SDL_Renderer* renderer, TTF_Font* font)
//     : active(false),
//       // inputText(""),
//       backgroundColor({0, 0, 0, 255}),
//       textColor({255, 255, 255, 255}),
//       textField(200 + 10, 200 + 10, 400 - 20, 200 - 20, font, textColor) {
//          dialogRect = {200, 200, 400, 200}; // Example position and size
//       }
//       // textRenderer(renderer, font) {
//       dialogRect = {200, 200, 400, 200}; // Example position and size
// }
Dialog::Dialog(SDL_Renderer* renderer, TTF_Font* font)
    : active(false),
      // inputText(""),
      backgroundColor({0, 0, 0, 255}),
      textColor({255, 255, 255, 255}),
      textField(200 + 10, 200 + 10, 100, 200 - 20, font, textColor, 5)
{
    dialogRect = {200, 200, 400, 200};
    // Отладочный вывод
    std::cout << "Dialog created with parameters:" << std::endl;
    std::cout << "  Renderer: " << renderer << std::endl;
    std::cout << "  Font: " << font << std::endl;
    std::cout << "  Dialog rect: { x: " << dialogRect.x << ", y: " << dialogRect.y << ", w: " << dialogRect.w << ", h: " << dialogRect.h << " }" << std::endl;
    std::cout << "  Background color: { r: " << (int)backgroundColor.r << ", g: " << (int)backgroundColor.g << ", b: " << (int)backgroundColor.b << ", a: " << (int)backgroundColor.a << " }" << std::endl;
    std::cout << "  Text color: { r: " << (int)textColor.r << ", g: " << (int)textColor.g << ", b: " << (int)textColor.b << ", a: " << (int)textColor.a << " }" << std::endl;
    std::cout << "  TextField parameters: { x: 210, y: 210, w: 380, h: 180, maxLines: 5 }" << std::endl;
}

void Dialog::show() {
    std::cerr << "-> Dialog show() [2]" << std::endl;
    active = true;
    textField.setFocus();
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


void Dialog::handleEvent(SDL_Event& event) {
    std::cerr << "-> Dialog::handleEvent [5]" << std::endl;
    if (!active) return;
    std::cerr << "-> Dialog::handleEvent+active [5]" << std::endl;

    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_ESCAPE) {
            hide();
        } // else if (event.key.keysym.sym == SDLK_BACKSPACE && !inputText.empty()) {
        //     std::cerr << inputText << std::endl;
        //     inputText.pop_back();
        // }
    // } else if (event.type == SDL_TEXTINPUT) {
    //     inputText += event.text.text;
    }

    textField.handleEvent(event);
}


void Dialog::render(SDL_Renderer* renderer) {
    // std::cerr << "-> Dialog::render  [9]" << std::endl;

    if (!active) return;

    // Render dialog background
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &dialogRect);

    // Render input text field
    textField.render(renderer);

    // // Render input text
    // int width, height;
    // SDL_Texture* textTexture = textRenderer.renderText(inputText, textColor, width, height);
    // if (textTexture) {
    //     SDL_Rect textRect = {dialogRect.x + 10, dialogRect.y + 10, width, height};
    //     SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
    //     SDL_DestroyTexture(textTexture);
    // }
}

void Dialog::setCloseCallback(std::function<void()> callback) {
    closeCallback = callback;
}
