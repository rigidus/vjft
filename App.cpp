// App.cpp

#include "App.hpp"
#include "KeyEvent.hpp"

App::App()
    : m_window(nullptr),
      m_renderer(nullptr),
      m_running(true)
{
    if (!initSDL() || !initTTF() || !initWindow() || !initRenderer()) {
        throw std::runtime_error("Failed to initialize SDL components");
    }

    m_stick_figure.emplace(m_renderer);
    m_player.emplace(*m_stick_figure);
    m_eventManager.addListener(&(*m_player));

    m_text_renderer.emplace(m_renderer, "16x8pxl-mono.ttf", 20);
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "TextRenderer destructor manually" << std::endl;
        m_text_renderer.reset();
    });
    std::cerr << "TextRenderer" << std::endl;


    SDL_Color color = {127, 127, 127, 127};
    int width, height;
    m_text_texture = m_text_renderer->renderText("Hello, SDL2!", color, width, height);
    if (!m_text_texture) {
        std::cerr << "Failed to render text texture" << std::endl;
        return;
    }
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "SDL_DestroyTexture(m_text_texture)" << std::endl;
        SDL_DestroyTexture(m_text_texture);
    });
    std::cerr << "m_test_texture" << std::endl;


    m_text_rect = {100, 100, width, height};  // Установка позиции и размера текста

    std::cout << ":: App initialized successfully." << std::endl;
}

App::~App()
{
    std::cout << ":: App destruction started" << std::endl;
    m_cleanupStack.executeAll();
    std::cout << ":: App destruction finished" << std::endl;
}


bool App::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }
    m_cleanupStack.addCleanupTask([]() {
        std::cerr << "SDL_Quit()" << std::endl;
        SDL_Quit();
    });
    std::cerr << "SDL_Init()" << std::endl;
    return true;
}


bool App::initTTF() {
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }
    m_cleanupStack.addCleanupTask([]() {
        std::cerr << "TTF_Quit()" << std::endl;
        TTF_Quit();
    });
    std::cerr << "TTF_Init()" << std::endl;
    return true;
}


bool App::initWindow() {
    m_window = SDL_CreateWindow("SDL2 Window", SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, 1024, 768, 0);
    if(!m_window)
    {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "SDL_DestroyWindow(m_window)" << std::endl;
        SDL_DestroyWindow(m_window);
    });
    std::cerr << "SDL_CreateWindow()" << std::endl;
    return true;
}

bool App::initRenderer() {
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return false;
    }
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "SDL_DestroyRenderer(m_renderer)" << std::endl;
        SDL_DestroyRenderer(m_renderer);
    });
    std::cerr << "SDL_CreateRenderer()" << std::endl;
    return true;
}


void App::processEvents() {
    SDL_Event sdlEvent;
    while(SDL_PollEvent(&sdlEvent) > 0)
    {
        if (sdlEvent.type == SDL_QUIT) {
            m_running = false;
        } else if (sdlEvent.type == SDL_KEYDOWN) {
            handleKeyPress(sdlEvent.key.keysym.sym);
        }
    }
}


void App::handleKeyPress(SDL_Keycode key) {
    switch (key) {
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
    }
}


void App::loop()
{
    const int targetFPS = 60;
    const int frameDelay = 1000 / targetFPS;
    Uint32 frameStart;
    int frameTime;
    while(m_running) {
        frameStart = SDL_GetTicks();
        processEvents();
        update(1.0 / targetFPS);
        draw();
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
}


void App::update(double delta_time)
{
    if (m_player) {
        m_player->update(delta_time);
    }
}


void App::draw()
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 0);
    SDL_RenderClear(m_renderer);

    if (m_stick_figure) {
        m_stick_figure->draw(m_renderer);
    }

    if (m_text_texture) {
        SDL_RenderCopy(m_renderer, m_text_texture, nullptr, &m_text_rect);
    }

    SDL_RenderPresent(m_renderer);
}
