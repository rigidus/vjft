// SpriteSheet.cpp

#include "SpriteSheet.hpp"

Spritesheet::Spritesheet(char const *path, int row, int column, SDL_Renderer* renderer)
{
    m_spritesheet_image = load_bmp(path);
    if (m_spritesheet_image == nullptr) {
        std::cerr << "Failed to load spritesheet image: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Failed to load spritesheet image");
    }

    std::cout << "Renderer: " << renderer << std::endl;

    m_texture = SDL_CreateTextureFromSurface(renderer, m_spritesheet_image);
    if (m_texture == nullptr) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(m_spritesheet_image);
        throw std::runtime_error("Failed to create texture from surface");
    }

    std::cout << "Column: " << column << std::endl;
    std::cout << "Row: " << row << std::endl;

    m_clip.w = m_spritesheet_image->w / column;
    m_clip.h = m_spritesheet_image->h / row;
}


Spritesheet::~Spritesheet()
{
    SDL_FreeSurface(m_spritesheet_image);
    SDL_DestroyTexture(m_texture);
}


void Spritesheet::select_sprite(int x, int y)
{
    m_clip.x = x * m_clip.w;
    m_clip.y = y * m_clip.h;
}


void Spritesheet::draw_selected_sprite(SDL_Renderer *renderer, SDL_Rect *position)
{
    SDL_RenderCopy(renderer, m_texture, &m_clip, position);
}
