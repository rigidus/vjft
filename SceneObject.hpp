// SceneObject.hpp
#pragma once

#include <SDL2/SDL.h>

class SceneObject {
public:
    virtual void update(double delta_time) = 0;
    virtual void draw(SDL_Renderer* renderer) = 0;
    virtual SDL_Rect getBoundingBox() const = 0;
};
