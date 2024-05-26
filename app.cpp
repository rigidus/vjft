#include "app.hpp"
#include "TextRenderer.hpp"
#include "stick_figure.hpp"
#include "EventManager.hpp"
#include "Player.hpp"
#include "KeyEvent.hpp"

App::App()
    : m_window(nullptr),
      m_renderer(nullptr), m_texture(nullptr),
      m_sprite_texture(nullptr),
      m_sprite_srcRect({0, 0, 64, 64}), m_sprite_dstRect({0, 0, 64, 64}),
      dstrect({0, 0, 0, 0}),
      m_running(true)
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

    m_player.emplace(*m_stick_figure);

    std::cerr << "HERE: m_player initialized" << std::endl;


    // Инициализация TextRenderer
    m_text_renderer = new TextRenderer(m_renderer);
    if (!m_text_renderer->loadFont("16x8pxl-mono.ttf", 24)) {
        std::cerr << "Failed to load font" << std::endl;
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }

    SDL_Color color = {127, 127, 127, 127};
    int width, height;
    m_text_texture = m_text_renderer->renderText("Hello, SDL2!", color, width, height);
    if (!m_text_texture) {
        std::cerr << "Failed to render text texture" << std::endl;
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        TTF_Quit();
        SDL_Quit();
        return;
    }
    m_text_rect = {100, 100, width, height};  // Установка позиции и размера текста


    m_eventManager.addListener(&(*m_player));

    std::cout << ":: App initialized successfully." << std::endl;
}

App::~App()
{
    SDL_DestroyTexture(m_text_texture);
    delete m_text_renderer;
    SDL_DestroyTexture(m_sprite_texture);
    SDL_DestroyTexture(m_texture);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    TTF_Quit();
    SDL_Quit();
}

void App::loop()
{
    const int targetFPS = 60;
    const int frameDelay = 1000 / targetFPS;
    Uint32 frameStart;
    int frameTime;
    SDL_Event sdlEvent;
    while(m_running) {
        frameStart = SDL_GetTicks();

        while(SDL_PollEvent(&sdlEvent) > 0)
        {
            if (sdlEvent.type == SDL_QUIT) {
                m_running = false;
            } else if (sdlEvent.type == SDL_KEYDOWN) {
                switch (sdlEvent.key.keysym.sym) {
                case SDLK_w:
                    m_eventManager.sendEvent(KeyEvent(KeyEvent::W));
                    break;
                case SDLK_a:
                    m_eventManager.sendEvent(KeyEvent(KeyEvent::A));
                    break;
                case SDLK_s:
                    m_eventManager.sendEvent(KeyEvent(KeyEvent::S));
                    break;
                case SDLK_d:
                    m_eventManager.sendEvent(KeyEvent(KeyEvent::D));
                    break;
                default:
                    continue; // Игнорируем другие клавиши
                }
            }
        }

        // update(1.0/60.0);
        update(1.0 / targetFPS); // Обновление с фиксированным временным шагом

        draw();

        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime); // Задержка для достижения целевого FPS
        }
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

    // Отрисовка текстуры спрайта
    // SDL_RenderCopy(m_renderer, m_sprite_texture, &m_sprite_srcRect, &m_sprite_dstRect);
    // Отрисовка текстуры текста
    SDL_RenderCopy(m_renderer, m_text_texture, nullptr, &m_text_rect);

    SDL_RenderPresent(m_renderer);
}
