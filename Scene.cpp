// Scene.cpp
#include "Scene.hpp"


Scene::Scene() {}


Scene::~Scene() {}


void Scene::addObject(std::shared_ptr<SceneObject> object) {
    objects.push_back(object);
}


void Scene::update(double delta_time) {
    for (auto& object : objects) {
        object->update(delta_time);
    }
}


void Scene::draw(SDL_Renderer* renderer, std::shared_ptr<Viewport> viewport) {
    for (const auto& object : this->getObjects()) {
        if (viewport->isVisible(object->getBoundingBox())) {
            object->draw(renderer, viewport);
        }
    }
    // // old:
    // for (auto& object : objects) {
    //     object->draw(renderer);
    // }
}


std::vector<std::shared_ptr<SceneObject>> Scene::getObjects() const {
    return objects;
}
