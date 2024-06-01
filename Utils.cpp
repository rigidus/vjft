// Utitls.cpp

#include "Utils.hpp"
#include <SDL2/SDL_image.h>
#include <iostream>


SDL_Surface *load_bmp(char const *path)
{
    SDL_Surface *image_surface = SDL_LoadBMP(path);

    if(!image_surface)
        return 0;

    return image_surface;
}


SDL_Surface* load_image(const char* path)
{
    SDL_Surface* image_surface = IMG_Load(path);
    if (!image_surface)
    {
        std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }
    return image_surface;
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

namespace Utils {
    void Bell() {
        std::cout << '\a';
    }
}
