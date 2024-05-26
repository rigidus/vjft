// StickFigure.cpp
#include "StickFigure.hpp"

int const SPRITESHEET_UP = 0;
int const SPRITESHEET_LEFT = 1;
int const SPRITESHEET_RIGHT = 2;
int const SPRITESHEET_DOWN = 3;

StickFigure::StickFigure(SDL_Renderer* renderer)
    : spriteSheet("spritesheet.bmp", 4, 9, renderer),
      spriteSheetColumn(0), direction(Direction::NONE),
      m_x(0), m_y(0)
{

    std::cerr << "HERE: m_stick_figure INSIDE" << std::endl;

    position = {0, 0, 22, 43};

    direction = Direction::NONE;

    spriteSheet.select_sprite(0, 0);
    spriteSheetColumn = 0;
}


void StickFigure::update(double delta_time)
{
    switch(direction)
    {
        case Direction::NONE:
            m_x += 0.0;
            m_y += 0.0;
            spriteSheet.select_sprite(0, 0);
            break;
        case Direction::UP:
            m_y = m_y - (500.0 * delta_time);
            spriteSheet.select_sprite(spriteSheetColumn, SPRITESHEET_UP);
            break;
        case Direction::DOWN:
            m_y = m_y + (500.0 * delta_time);
            spriteSheet.select_sprite(spriteSheetColumn, SPRITESHEET_DOWN);
            break;
        case Direction::LEFT:
            m_x = m_x - (500.0 * delta_time);
            spriteSheet.select_sprite(spriteSheetColumn, SPRITESHEET_LEFT);
            break;
        case Direction::RIGHT:
            m_x = m_x + (500.0 * delta_time);
            spriteSheet.select_sprite(spriteSheetColumn, SPRITESHEET_RIGHT);
            break;
    }

    position.x = m_x;
    position.y = m_y;

    spriteSheetColumn++;
    if(spriteSheetColumn > 8) {
        spriteSheetColumn = 1;
    }
}

void StickFigure::draw(SDL_Renderer *renderer)
{
    if (renderer == nullptr) {
        std::cerr << "Invalid renderer passed to StickFigure::draw" << std::endl;
        return;
    }

    spriteSheet.draw_selected_sprite(renderer, &position);

    // std::cout << "Drawing sprite at position (" << position.x << ", " << position.y << ")" << std::endl;
}


void StickFigure::moveUp()
{
    direction = Direction::UP;
}

void StickFigure::moveDown()
{
    direction = Direction::DOWN;
}

void StickFigure::moveLeft()
{
    direction = Direction::LEFT;
}

void StickFigure::moveRight()
{
    direction = Direction::RIGHT;
}
