// EventListener.hpp
#pragma once
#include "Event.hpp"
#include "Viewport.hpp"

class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const Event& event) = 0;
    virtual void draw(SDL_Renderer* renderer, std::shared_ptr<Viewport> viewport) = 0;
    virtual SDL_Rect getBoundingBox() const = 0;
};
