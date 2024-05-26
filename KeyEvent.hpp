// KeyEvent.hpp
#pragma once
#include "Event.hpp"

class KeyEvent : public Event {
public:
    KeyEvent(int keyCode) : keyCode(keyCode) {}
    int getKeyCode() const { return keyCode; }

private:
    int keyCode;
};
