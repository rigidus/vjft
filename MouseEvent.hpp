// MouseEvent.hpp
#pragma once
#include "Event.hpp"

class MouseEvent : public Event {
public:
    enum Button {
        LEFT,
        MIDDLE,
        RIGHT
    };

    MouseEvent(int x, int y, Button button, bool pressed)
        : x(x), y(y), button(button), pressed(pressed) {}

    int getX() const { return x; }
    int getY() const { return y; }
    Button getButton() const { return button; }
    bool isPressed() const { return pressed; }

private:
    int x, y;
    Button button;
    bool pressed;
};
