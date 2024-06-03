// Player.cpp
#include "Player.hpp"
#include <iostream>

Player::Player(Figure& figure, const std::vector<KeyEvent::KeyCode>& keys, const std::string& name)
    : figure(figure), controlKeys(keys), playerName(name) {}


void Player::onEvent(const Event& event) {
    KeyEvent* keyEvent = dynamic_cast<const KeyEvent*>(&event);
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


void Player::update(double delta_time) {
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


void Player::draw(SDL_Renderer* renderer// , Viewport& viewport
    ) {
    SDL_Rect renderRect = {
        getBoundingBox().x - viewport.getX(),
        getBoundingBox().y - viewport.getY(),
        getBoundingBox().w,
        getBoundingBox().h
    };
    figure.draw(renderer, viewport);
}


SDL_Rect Player::getBoundingBox() const {
    return figure.getBoundingBox();
}


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


void printActiveKeys() const {
    std::cout << playerName << " active keys: ";
    for (const auto& key : activeKeys) {
        std::cout << keyCodeToString(key) << " ";
    }
    std::cout << std::endl;
}
