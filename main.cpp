#include <iostream>
#include <SDL.h>
#include "App.hpp"
#include "TextRenderer.hpp"
#include "EventManager.hpp"
#include "Player.hpp"
#include "KeyEvent.hpp"

int main(int argc, char *argv[])
{
    App app;
    app.loop();
    return 0;
}
