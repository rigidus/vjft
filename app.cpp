#include "app.hpp"

App::App()
    : m_stick_figure(m_renderer)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return;
    }

    m_window = SDL_CreateWindow("SDL2 Window",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                1024, 768,
                                0);

    if(!m_window)
    {
        std::cout << "Failed to create window\n";
        std::cout << "SDL2 Error: " << SDL_GetError() << "\n";
        TTF_Quit();
        SDL_Quit();
        return;
    }

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (m_renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        return;
    }

    m_font = TTF_OpenFont("16x8pxl-mono.ttf", 24);
    if (m_font == nullptr) {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }

    // ------------------------

    SDL_Surface* surface_bmp = SDL_LoadBMP("spritesheet.bmp");
    if (!surface_bmp) {
        std::cerr << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface_bmp);
    SDL_FreeSurface(surface_bmp); // Освобождаем поверхность после создания текстуры
    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        return;
    }

    sprite_srcRect.x = 0;   // Начальная позиция по X на спрайтовом листе
    sprite_srcRect.y = 0;   // Начальная позиция по Y на спрайтовом листе
    sprite_srcRect.w = 64;  // Ширина спрайта
    sprite_srcRect.h = 64;  // Высота спрайта

    sprite_dstRect.x = 0; // Позиция по X на экране
    sprite_dstRect.y = 0; // Позиция по Y на экране
    sprite_dstRect.w = sprite_srcRect.w; // Ширина спрайта на экране
    sprite_dstRect.h = sprite_srcRect.h; // Высота спрайта на экране


    // -------------------------


    SDL_Color color = {127, 127, 127, 127};

    SDL_Surface* surface = TTF_RenderText_Solid(m_font, "Hello, SDL2!", color);
    if (surface == nullptr) {
        std::cerr << "TTF_RenderText_Solid Error: " << TTF_GetError() << std::endl;
        TTF_CloseFont(m_font);
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }

    m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);

    SDL_FreeSurface(surface);

    if (m_texture == nullptr) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        TTF_CloseFont(m_font);
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }

    // Инициализация dstrect
    dstrect.x = 100;
    dstrect.y = 100;
    dstrect.w = 200; // ширина текста
    dstrect.h = 50;  // высота текста
}

App::~App()
{
    SDL_DestroyTexture(m_texture);
    TTF_CloseFont(m_font);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    TTF_Quit();
    SDL_Quit();
}

void App::loop()
{
    bool keep_window_open = true;
    while(keep_window_open)
    {
        while(SDL_PollEvent(&m_window_event) > 0)
        {
            m_stick_figure.handle_events(m_window_event);
            switch(m_window_event.type)
            {
                case SDL_QUIT:
                    keep_window_open = false;
                    break;
            }
        }

        update(1.0/60.0);
        draw();
    }
}

void App::update(double delta_time)
{
    m_stick_figure.update(delta_time);
}


void App::draw()
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 0);
    SDL_RenderClear(m_renderer);

    // m_stick_figure.draw(m_renderer);

    // Сначала отрисовка спрайта
    SDL_RenderCopy(m_renderer, m_sprite_texture, &sprite_srcRect, &sprite_dstRect);

    // SDL_Delay(3000);

    SDL_RenderCopy(m_renderer, m_texture, nullptr, &dstrect);
    SDL_RenderPresent(m_renderer);
}
