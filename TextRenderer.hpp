#ifndef TEXTRENDERER_HPP
#define TEXTRENDERER_HPP

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <iostream>

class TextRenderer {
public:
    TextRenderer(SDL_Renderer* renderer);
    ~TextRenderer();

    bool loadFont(const std::string& path, int fontSize);
    SDL_Texture* renderText(const std::string& text, SDL_Color color, int& width, int& height);

private:
    SDL_Renderer* renderer;
    TTF_Font* font;
};

#endif // TEXTRENDERER_HPP
