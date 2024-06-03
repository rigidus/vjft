#include "SceneObject.hpp"

class SomeObject : public SceneObject {
public:
    void update(double delta_time) override {
        // Обновление состояния объекта
    }

    void draw(SDL_Renderer* renderer// , Viewport& viewport
        ) override {
        SDL_Rect boundingBox = getBoundingBox();
        SDL_Rect renderRect = {
            boundingBox.x - viewport.getX(),
            boundingBox.y - viewport.getY(),
            boundingBox.w,
            boundingBox.h
        };
        // Отрисовка объекта с учетом смещения Viewport
        SDL_RenderCopy(renderer, texture, nullptr, &renderRect);
    }

    SDL_Rect getBoundingBox() override {
        return { x, y, width, height }; // Возврат координат и размеров объекта
    }

private:
    int x, y, width, height;
    SDL_Texture* texture;
};
