#pragma once

#include <iostream>
#include <SDL2/SDL.h>
#include "utilities.hpp"

class Spritesheet
{
public:
    Spritesheet(char const *path, int row, int column, SDL_Renderer* renderer);
    ~Spritesheet();

    void select_sprite(int x, int y);
    void draw_selected_sprite(SDL_Renderer *renderer, SDL_Rect *position);
private:
    SDL_Rect     m_clip;
    SDL_Surface *m_spritesheet_image;
    SDL_Texture *m_texture;

    int m_start_x;
    int m_start_y;
};
