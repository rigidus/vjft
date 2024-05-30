// TextRenderer.cpp
#include "TextRenderer.hpp"


TextRenderer::TextRenderer(SDL_Renderer* renderer, TTF_Font* font)
    : renderer(renderer), m_font(font)
{
    if (!m_font)
    {
        std::cerr << "Font is nullptr!" << std::endl;
        throw std::runtime_error("Font is nullptr!");
    }
}


TextRenderer::~TextRenderer()
{
}


SDL_Texture* TextRenderer::renderText(const std::string& text, SDL_Color color, int& width, int& height)
{
    if (!m_font) {
        std::cerr << "Font not loaded!" << std::endl;
        return nullptr;
    }

    SDL_Surface* surface;
    if (text.empty()) {
        surface = TTF_RenderText_Solid(m_font, " ", color);
    } else {
        surface = TTF_RenderText_Solid(m_font, text.c_str(), color);
    }
    if (!surface) {
        std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(surface);
        return nullptr;
    }

    width = surface->w;
    height = surface->h;
    SDL_FreeSurface(surface);

    return texture;
}


TTF_Font* TextRenderer::getFont() const  // Реализация метода
{
    return m_font;
}
