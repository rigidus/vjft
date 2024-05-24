#include <iostream>
#include <SDL.h>
#include "app.hpp"
#include "TextRenderer.hpp"

int main(int argc, char *argv[])
{
    App app;
    app.loop();
    return 0;
}


// #include <SDL.h>
// #include <SDL_ttf.h>
// #include "TextRenderer.hpp"

// TextRenderer::TextRenderer(SDL_Renderer* renderer) : renderer(renderer), font(nullptr) {
//     if (TTF_Init() == -1) {
//         std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
//     }
// }

// TextRenderer::~TextRenderer() {
//     if (font) {
//         TTF_CloseFont(font);
//     }
//     TTF_Quit();
// }

// bool TextRenderer::loadFont(const std::string& path, int fontSize) {
//     font = TTF_OpenFont(path.c_str(), fontSize);
//     if (!font) {
//         std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
//         return false;
//     }
//     return true;
// }

// SDL_Texture* TextRenderer::renderText(const std::string& text, SDL_Color color, int& width, int& height) {
//     if (!font) {
//         std::cerr << "Font not loaded!" << std::endl;
//         return nullptr;
//     }

//     SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
//     if (!surface) {
//         std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
//         return nullptr;
//     }

//     SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
//     if (!texture) {
//         std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
//         SDL_FreeSurface(surface);
//         return nullptr;
//     }

//     width = surface->w;
//     height = surface->h;
//     SDL_FreeSurface(surface);
//     return texture;
// }

// int main(int argc, char* argv[]) {
//     if (SDL_Init(SDL_INIT_VIDEO) != 0) {
//         std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
//         return 1;
//     }

//     SDL_Window* window = SDL_CreateWindow("SDL2 TTF Example",
//                                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
//                                           800, 600, SDL_WINDOW_SHOWN);
//     if (!window) {
//         std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
//         SDL_Quit();
//         return 1;
//     }

//     SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
//     if (!renderer) {
//         std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
//         SDL_DestroyWindow(window);
//         SDL_Quit();
//         return 1;
//     }

//     TextRenderer textRenderer(renderer);
//     if (!textRenderer.loadFont("16x8pxl-mono.ttf", 24)) {
//         SDL_DestroyRenderer(renderer);
//         SDL_DestroyWindow(window);
//         SDL_Quit();
//         return 1;
//     }

//     SDL_Color color = {255, 255, 255, 255};
//     int width, height;
//     SDL_Texture* texture = textRenderer.renderText("Hello, SDL2!", color, width, height);
//     if (!texture) {
//         SDL_DestroyRenderer(renderer);
//         SDL_DestroyWindow(window);
//         SDL_Quit();
//         return 1;
//     }

//     SDL_Rect dstrect = {100, 100, width, height};

//     SDL_Event e;
//     bool quit = false;
//     while (!quit) {
//         while (SDL_PollEvent(&e)) {
//             if (e.type == SDL_QUIT) {
//                 quit = true;
//             }
//         }

//         SDL_RenderClear(renderer);
//         SDL_RenderCopy(renderer, texture, nullptr, &dstrect);
//         SDL_RenderPresent(renderer);
//     }

//     SDL_DestroyTexture(texture);
//     SDL_DestroyRenderer(renderer);
//     SDL_DestroyWindow(window);
//     SDL_Quit();

//     return 0;
// }
