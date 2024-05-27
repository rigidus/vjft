// Player.hpp
#pragma once
#include "EventListener.hpp"
#include "KeyEvent.hpp"
#include "StickFigure.hpp"
#include <string>
#include <vector>
#include <unordered_set>

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
            if (keyEvent.isPressed()) {
                activeKeys.insert(keyCode);
            } else {
                activeKeys.erase(keyCode);
            }
            printActiveKeys();
        }
    }

    void update(double delta_time) {
        if (activeKeys.empty()) {
            stickFigure.setDirection(StickFigure::Direction::NONE);
        } else {
            if (activeKeys.count(KeyEvent::W) || activeKeys.count(KeyEvent::I)) {
                std::cout << playerName << " moves up" << std::endl;
                stickFigure.moveUp();
            }
            if (activeKeys.count(KeyEvent::A) || activeKeys.count(KeyEvent::J)) {
                std::cout << playerName << " moves left" << std::endl;
                stickFigure.moveLeft();
            }
            if (activeKeys.count(KeyEvent::S) || activeKeys.count(KeyEvent::K)) {
                std::cout << playerName << " moves down" << std::endl;
                stickFigure.moveDown();
            }
            if (activeKeys.count(KeyEvent::D) || activeKeys.count(KeyEvent::L)) {
                std::cout << playerName << " moves right" << std::endl;
                stickFigure.moveRight();
            }
        }
        stickFigure.update(delta_time);
    }

    void printActiveKeys() const {
        std::cout << playerName << " active keys: ";
        for (const auto& key : activeKeys) {
            std::cout << keyCodeToString(key) << " ";
        }
        std::cout << std::endl;
    }

private:
    StickFigure& stickFigure;
    std::vector<KeyEvent::KeyCode> controlKeys;
    std::unordered_set<KeyEvent::KeyCode> activeKeys;
    std::string playerName;

    std::string keyCodeToString(KeyEvent::KeyCode keyCode) const {
        switch (keyCode) {
        case KeyEvent::W: return "W";
        case KeyEvent::A: return "A";
        case KeyEvent::S: return "S";
        case KeyEvent::D: return "D";
        case KeyEvent::I: return "I";
        case KeyEvent::J: return "J";
        case KeyEvent::K: return "K";
        case KeyEvent::L: return "L";
        default: return "Unknown";
        }
    }
};
