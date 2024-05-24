#include "TextRenderer.hpp"

TextRenderer::TextRenderer(SDL_Renderer* renderer)
    : renderer(renderer), font(nullptr)
{
    if (TTF_Init() == -1)
    {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
    }
}

TextRenderer::~TextRenderer()
{
    if (font)
    {
        TTF_CloseFont(font);
    }
    TTF_Quit();
}

bool TextRenderer::loadFont(const std::string& path, int fontSize)
{
    font = TTF_OpenFont(path.c_str(), fontSize);
    if (!font)
    {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        return false;
    }
    return true;
}

SDL_Texture* TextRenderer::renderText(const std::string& text, SDL_Color color, int& width, int& height)
{
    if (!font)
    {
        std::cerr << "Font not loaded!" << std::endl;
        return nullptr;
    }

    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface)
    {
        std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture)
    {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return nullptr;
    }

    width = surface->w;
    height = surface->h;
    SDL_FreeSurface(surface);
    return texture;
}
