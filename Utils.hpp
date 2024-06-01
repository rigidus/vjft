// Utils.hpp

#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <sstream>

SDL_Surface *load_bmp(char const *path);
SDL_Surface* load_image(const char* path);
std::vector<std::string> splitString(const std::string&, char);

namespace Utils {
    void Bell();
}
