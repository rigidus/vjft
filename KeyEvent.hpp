// KeyEvent.hpp
#pragma once
#include "Event.hpp"

class KeyEvent : public Event {
public:
    enum KeyCode {
        W, A, S, D, I, J, K, L
    };

    KeyEvent(int keyCode) : keyCode(keyCode) {}
    int getKeyCode() const { return keyCode; }

private:
    int keyCode;
};
