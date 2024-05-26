// Player.hpp
#pragma once
#include "EventListener.hpp"
#include "KeyEvent.hpp"
#include "stick_figure.hpp"
#include <iostream>

class Player : public EventListener {
public:
    Player(StickFigure& stickFigure) : stickFigure(stickFigure) {}

    void onEvent(const Event& event) override {
        const KeyEvent* keyEvent = dynamic_cast<const KeyEvent*>(&event);
        if (keyEvent) {
            handleKeyEvent(*keyEvent);
        }
    }

    void handleKeyEvent(const KeyEvent& keyEvent) {
        KeyEvent::KeyCode keyCode = (KeyEvent::KeyCode)keyEvent.getKeyCode();
        switch (keyCode) {
        case KeyEvent::W:
            std::cout << "Player moves up" << std::endl;
            stickFigure.moveUp();
            break;
        case KeyEvent::A:
            std::cout << "Player moves left" << std::endl;
            stickFigure.moveLeft();
            break;
        case KeyEvent::S:
            std::cout << "Player moves down" << std::endl;
            stickFigure.moveDown();
            break;
        case KeyEvent::D:
            std::cout << "Player moves right" << std::endl;
            stickFigure.moveRight();
            break;
        }
    }

private:
    StickFigure& stickFigure;
};
