// Player.hpp
#pragma once
#include "EventListener.hpp"
#include "KeyEvent.hpp"
#include "Figure.hpp"
#include "SceneObject.hpp"
#include "MouseEvent.hpp"
#include <string>
#include <vector>
#include <unordered_set>

class Player : public EventListener, public SceneObject {
public:
    Player(Figure& figure,
           const std::vector<KeyEvent::KeyCode>& keys,
           const std::string& name)
        : figure(figure),
          controlKeys(keys),
          playerName(name) {}


    void onEvent(const Event& event) override {
        const KeyEvent* keyEvent = dynamic_cast<const KeyEvent*>(&event);
        if (keyEvent) {
            handleKeyEvent(*keyEvent);
        } else if (const MouseEvent* mouseEvent = dynamic_cast<const MouseEvent*>(&event)) {
            handleMouseEvent(*mouseEvent);
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

    void handleMouseEvent(const MouseEvent& mouseEvent) {
        if (mouseEvent.isPressed()) {
            std::cout << playerName << " mouse clicked at (" << mouseEvent.getX() << ", " << mouseEvent.getY() << ")" << std::endl;
        } else {
            std::cout << playerName << " mouse released at (" << mouseEvent.getX() << ", " << mouseEvent.getY() << ")" << std::endl;
        }
    }


    void update(double delta_time) override {
        if (activeKeys.empty()) {
            figure.setDirection(Figure::Direction::NONE);
        } else {
            if (activeKeys.count(KeyEvent::W) || activeKeys.count(KeyEvent::I)) {
                std::cout << playerName << " moves up" << std::endl;
                figure.moveUp();
            }
            if (activeKeys.count(KeyEvent::A) || activeKeys.count(KeyEvent::J)) {
                std::cout << playerName << " moves left" << std::endl;
                figure.moveLeft();
            }
            if (activeKeys.count(KeyEvent::S) || activeKeys.count(KeyEvent::K)) {
                std::cout << playerName << " moves down" << std::endl;
                figure.moveDown();
            }
            if (activeKeys.count(KeyEvent::D) || activeKeys.count(KeyEvent::L)) {
                std::cout << playerName << " moves right" << std::endl;
                figure.moveRight();
            }
        }
        figure.update(delta_time);
    }

    void draw(SDL_Renderer* renderer, std::shared_ptr<Viewport> viewport) override {
        figure.draw(renderer, viewport);
    }


    SDL_Rect getBoundingBox() const override {
        return figure.getBoundingBox();
    }


    void printActiveKeys() const {
        std::cout << playerName << " active keys: ";
        for (const auto& key : activeKeys) {
            std::cout << keyCodeToString(key) << " ";
        }
        std::cout << std::endl;
    }

private:
    Figure& figure;
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
