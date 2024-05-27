// Scene.hpp
#pragma once

#include <vector>
#include <SDL2/SDL.h>
#include <memory>
#include "SceneObject.hpp"
#include "Viewport.hpp"

class Scene {
public:
    Scene();
    ~Scene();

    void addObject(std::shared_ptr<SceneObject> object);
    void update(double delta_time);
    void draw(SDL_Renderer* renderer, std::shared_ptr<Viewport> viewport);
    std::vector<std::shared_ptr<SceneObject>> getObjects() const;

private:
    std::vector<std::shared_ptr<SceneObject>> objects;
};
