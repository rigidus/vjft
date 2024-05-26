// Player.hpp
#pragma once
#include "EventListener.hpp"
#include "KeyEvent.hpp"
#include <iostream>

class Player : public EventListener {
public:
    void onEvent(const Event& event) override {
        const KeyEvent* keyEvent = dynamic_cast<const KeyEvent*>(&event);
        if (keyEvent) {
            handleKeyEvent(*keyEvent);
        }
    }

    void handleKeyEvent(const KeyEvent& keyEvent) {
        int keyCode = keyEvent.getKeyCode();
        if (keyCode == 87) { // 'W' key
            std::cout << "Player moves up" << std::endl;
        }
    }
};
