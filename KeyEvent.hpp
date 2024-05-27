// KeyEvent.hpp
#pragma once
#include "Event.hpp"

class KeyEvent : public Event {
public:
    enum KeyCode {
        W, A, S, D, I, J, K, L
    };

    KeyEvent(int keyCode, bool pressed) : keyCode(keyCode), pressed(pressed) {}
    int getKeyCode() const { return keyCode; }
    bool isPressed() const { return pressed; }

private:
    int keyCode;
    bool pressed;
};
