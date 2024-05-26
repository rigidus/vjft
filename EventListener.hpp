// EventListener.hpp
#pragma once
#include "Event.hpp"

class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const Event& event) = 0;
};
