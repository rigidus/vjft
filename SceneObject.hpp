// SceneObject.hpp
#pragma once

#include <SDL2/SDL.h>
#include "Viewport.hpp"

class SceneObject {
public:
    virtual void update(double delta_time) = 0;
    virtual void draw(SDL_Renderer* renderer, std::shared_ptr<Viewport> viewport) = 0;
    virtual SDL_Rect getBoundingBox() const = 0;
};
