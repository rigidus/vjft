// Player.hpp
#pragma once
#include "EventListener.hpp"
#include "KeyEvent.hpp"
#include "StickFigure.hpp"
#include <string>

class Player : public EventListener {
public:
    Player(StickFigure& stickFigure,
           const std::vector<KeyEvent::KeyCode>& keys,
           const std::string& name)
        : stickFigure(stickFigure),
          controlKeys(keys),
          playerName(name) {}

    void onEvent(const Event& event) override {
        const KeyEvent* keyEvent = dynamic_cast<const KeyEvent*>(&event);
        if (keyEvent) {
            handleKeyEvent(*keyEvent);
        }
    }

    void handleKeyEvent(const KeyEvent& keyEvent) {
        KeyEvent::KeyCode keyCode = static_cast<KeyEvent::KeyCode>(keyEvent.getKeyCode());
        if (std::find(controlKeys.begin(), controlKeys.end(), keyCode) !=
            controlKeys.end()) {
            switch (keyCode) {
            case KeyEvent::W:
            case KeyEvent::I:
                std::cout << playerName << " moves up" << std::endl;
                stickFigure.moveUp();
                break;
            case KeyEvent::A:
            case KeyEvent::J:
                std::cout << playerName << " moves left" << std::endl;
                stickFigure.moveLeft();
                break;
            case KeyEvent::S:
            case KeyEvent::K:
                std::cout << playerName << " moves down" << std::endl;
                stickFigure.moveDown();
                break;
            case KeyEvent::D:
            case KeyEvent::L:
                std::cout << playerName << " moves right" << std::endl;
                stickFigure.moveRight();
                break;
            }
        }
    }

    void update(double delta_time) {
        stickFigure.update(delta_time);
    }


private:
    StickFigure& stickFigure;
    std::vector<KeyEvent::KeyCode> controlKeys;
    std::string playerName;
};
