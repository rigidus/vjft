// TextRenderer.cpp
#include "TextRenderer.hpp"

TextRenderer::TextRenderer(SDL_Renderer* renderer, const std::string& path, int fontSize)
    : renderer(renderer), m_font(nullptr)
{
    m_font = TTF_OpenFont(path.c_str(), fontSize);
    if (!m_font)
    {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        throw std::runtime_error(std::string("TTF_OpenFont Error: ") + TTF_GetError());
    }

}

TextRenderer::~TextRenderer()
{
    if (m_font) {
        TTF_CloseFont(m_font);
    }
}


SDL_Texture* TextRenderer::renderText(const std::string& text, SDL_Color color, int& width, int& height)
{
    if (!m_font) {
        std::cerr << "Font not loaded!" << std::endl;
        return nullptr;
    }

    SDL_Surface* surface = TTF_RenderText_Solid(m_font, text.c_str(), color);
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
