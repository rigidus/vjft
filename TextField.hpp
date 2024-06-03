// TextField.hpp

#pragma once

#include <SDL.h>
#include <iostream>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "Utils.hpp"

class TextField {
public:
    TextField(int x, int y, int w, int h, TTF_Font* font, SDL_Color textColor, int maxLines);
    void handleEvent(SDL_Event& e);
    void render(SDL_Renderer* renderer);
    // void addText(const std::string& newText);
    void setFocus();
    void loseFocus();
    bool hasFocus() const;
    void setInitialLine(const std::string& line);

    std::vector<std::string> splitString(const std::string& input);
    std::vector<std::string> wrapSplitted(const std::vector<std::string>& input);

    int x, y, width, height, maxLines;;
    TTF_Font* font;
    SDL_Color textColor;
    // std::string text;
    std::list<std::string> lines;
    bool focused;
    int cursor;

    // void wrapText();

    void wrapLine();
    void onFocus();
    void onLoseFocus();
private:
};
