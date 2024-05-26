// EventManager.hpp
#pragma once
#include <vector>
#include <algorithm>
#include "EventListener.hpp"

class EventManager {
public:
    void addListener(EventListener* listener) {
        listeners.push_back(listener);
    }

    void removeListener(EventListener* listener) {
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    }

    void sendEvent(const Event& event) {
        for (auto listener : listeners) {
            listener->onEvent(event);
        }
    }

private:
    std::vector<EventListener*> listeners;
};
