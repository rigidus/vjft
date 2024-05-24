#include "app.hpp"
#include "TextRenderer.hpp"
#include "stick_figure.hpp"

App::App()
    : m_window(nullptr),
      m_renderer(nullptr), m_texture(nullptr),
      m_sprite_texture(nullptr), // m_stick_figure(nullptr),
      m_sprite_srcRect({0, 0, 64, 64}), m_sprite_dstRect({0, 0, 64, 64}),
      dstrect({0, 0, 0, 0})
{
    std::cerr << "HERE: App ctcr" << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
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

    std::cerr << "HERE: Window" << std::endl;

    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (m_renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        return;
    }

    std::cout << "Renderer created successfully: " << m_renderer << std::endl;

    m_stick_figure.emplace(m_renderer);

    std::cerr << "HERE: m_stick_figure initialized" << std::endl;


    // // ------------------------

    // SDL_Surface* surface_bmp = SDL_LoadBMP("spritesheet.bmp");
    // if (!surface_bmp) {
    //     std::cerr << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
    //     SDL_DestroyRenderer(m_renderer);
    //     SDL_DestroyWindow(m_window);
    //     TTF_Quit();
    //     SDL_Quit();
    //     return;
    // }

    // m_sprite_texture = SDL_CreateTextureFromSurface(m_renderer, surface_bmp);
    // SDL_FreeSurface(surface_bmp); // Освобождаем поверхность после создания текстуры
    // if (!m_sprite_texture) {
    //     std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
    //     SDL_DestroyRenderer(m_renderer);
    //     SDL_DestroyWindow(m_window);
    //     TTF_Quit();
    //     SDL_Quit();
    //     return;
    // }

    // // Начальная позиция и размер спрайта
    // m_sprite_srcRect = {0, 0, 64, 64};
    // // Позиция и размер спрайта на экране
    // m_sprite_dstRect = {0, 0, m_sprite_srcRect.w, m_sprite_srcRect.h} ;

    // // -------------------------

    TextRenderer textRenderer(m_renderer);
    if (!textRenderer.loadFont("16x8pxl-mono.ttf", 24)) {
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }

    SDL_Color color = {127, 127, 127, 127};
    int width, height;
    m_texture = textRenderer.renderText("Hello, SDL2!", color, width, height);
    if (!m_texture) {
        std::cerr << "Failed to load text texture\n";
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }
    SDL_Rect dstrect = {100, 100, width, height};
}

App::~App()
{
    SDL_DestroyTexture(m_texture);
    SDL_DestroyTexture(m_sprite_texture);
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
            if (m_stick_figure) {
                m_stick_figure->handle_events(m_window_event);
            }
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
    if (m_stick_figure) {
        m_stick_figure->update(delta_time);
    }
}

void App::draw()
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 0);
    SDL_RenderClear(m_renderer);

    if (m_stick_figure) {
        m_stick_figure->draw(m_renderer);
    }

    // Сначала отрисовка спрайта
    SDL_RenderCopy(m_renderer, m_sprite_texture, &m_sprite_srcRect, &m_sprite_dstRect);

    // SDL_Delay(3000);

    SDL_RenderCopy(m_renderer, m_texture, nullptr, &dstrect);
    SDL_RenderPresent(m_renderer);
}
