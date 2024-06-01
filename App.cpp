// App.cpp

#include "App.hpp"
#include "KeyEvent.hpp"
#include "Scene.hpp"
// #include <new>

App::App()
    : m_window(nullptr),
      m_renderer(nullptr),
      m_running(true),
      m_viewport(std::make_shared<Viewport>(0, 0, 1024, 768)),
      m_dialogActive(false)
{
    if ( !initSDL() || !initIMG() || !initTTF() || !initWindow() || !initRenderer() ||
         !initFont() ) {
        Cleanup();
        throw std::runtime_error("Failed to initialize SDL components");
    }

    m_scene = Scene();

    m_figure1 =
        std::make_shared<Figure>(m_renderer, "sprites_orange.png", 100, 100);
    m_figure2 =
        std::make_shared<Figure>(m_renderer, "sprites_blue.png", 300, 300);

    m_player1 = std::make_shared<Player>(
        *m_figure1,
        std::vector<KeyEvent::KeyCode>{
            KeyEvent::W, KeyEvent::A, KeyEvent::S, KeyEvent::D},
        "Player1");
    m_player2 = std::make_shared<Player>(
        *m_figure2,
        std::vector<KeyEvent::KeyCode>{
            KeyEvent::I, KeyEvent::J, KeyEvent::K, KeyEvent::L},
        "Player2");

    m_scene.addObject(m_player1);
    m_scene.addObject(m_player2);

    m_eventManager.addListener(&(*m_player1));
    m_eventManager.addListener(&(*m_player2));

    m_text_renderer.emplace(m_renderer, m_font);
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "TextRenderer destructor manually" << std::endl;
        m_text_renderer.reset();
    });
    std::cerr << "TextRenderer" << std::endl;

    SDL_Color color = {127, 127, 127, 127};
    int width, height;
    m_text_texture = m_text_renderer->renderText("Control: WASD, IJKL", color, width, height);
    if (!m_text_texture) {
        std::cerr << "Failed to render text texture" << std::endl;
        Cleanup();
        return;
    }
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "SDL_DestroyTexture(m_text_texture)" << std::endl;
        SDL_DestroyTexture(m_text_texture);
    });
    std::cerr << "m_test_texture" << std::endl;

    m_text_rect = {100, 100, width, height};

    m_dialog = std::make_unique<Dialog>(m_renderer, m_text_renderer->getFont());
    m_dialog->setCloseCallback([this]() { onDialogClose(); });

    std::cout << ":: App initialized successfully." << std::endl;
}


App::~App() {}


void App::Cleanup()
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


bool App::initIMG() {
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        return false;
    }
    m_cleanupStack.addCleanupTask([]() {
        std::cerr << "IMG_Quit()" << std::endl;
        IMG_Quit();
    });
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
    if(!m_window) {
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

bool App::initFont() {
    m_font = TTF_OpenFont("16x8pxl-mono.ttf", 20);
    if (!m_font)
    {
        std::cerr << "TTF_OpenFont Error: " << TTF_GetError() << std::endl;
        throw std::runtime_error(std::string("TTF_OpenFont Error: ") + TTF_GetError());
    }
    m_cleanupStack.addCleanupTask([this]() {
        std::cerr << "TTF_CloseFont()" << std::endl;
        TTF_CloseFont(m_font);
    });
    std::cerr << "TTF_OpenFont()" << std::endl;
    return true;
}



void App::handleKeyPress(SDL_Keycode key) {
    if (key == SDLK_g) {
        m_dialog->show();
        m_dialogActive = true;
    } else {
        switch (key) {
        case SDLK_w:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::W, true));
            break;
        case SDLK_a:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::A, true));
            break;
        case SDLK_s:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::S, true));
            break;
        case SDLK_d:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::D, true));
            break;
        case SDLK_i:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::I, true));
            break;
        case SDLK_j:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::J, true));
            break;
        case SDLK_k:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::K, true));
            break;
        case SDLK_l:
            m_eventManager.sendEvent(KeyEvent(KeyEvent::L, true));
            break;
        }
    }
}


void App::handleKeyRelease(SDL_Keycode key) {
    switch (key) {
    case SDLK_w:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::W, false));
        break;
    case SDLK_a:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::A, false));
        break;
    case SDLK_s:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::S, false));
        break;
    case SDLK_d:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::D, false));
        break;
    case SDLK_i:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::I, false));
        break;
    case SDLK_j:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::J, false));
        break;
    case SDLK_k:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::K, false));
        break;
    case SDLK_l:
        m_eventManager.sendEvent(KeyEvent(KeyEvent::L, false));
        break;
    }
}


void App::handleMousePress(int x, int y, MouseEvent::Button button) {
    m_eventManager.sendEvent(MouseEvent(x, y, button, true));
}

void App::handleMouseRelease(int x, int y, MouseEvent::Button button) {
    m_eventManager.sendEvent(MouseEvent(x, y, button, false));
}


void App::processEvents() {
    SDL_Event sdlEvent;
    while(SDL_PollEvent(&sdlEvent) > 0)
    {
        if (sdlEvent.type == SDL_QUIT) {
            m_running = false;
        } else {
            if (m_dialogActive) {
                m_dialog->handleEvent(sdlEvent);
            } else {
                if (sdlEvent.type == SDL_KEYDOWN) {
                    std::cerr << "Press: " << sdlEvent.key.keysym.sym << std::endl;
                    if (true == m_dialogActive) {
                        m_dialog->handleEvent(sdlEvent);
                    } else {
                        handleKeyPress(sdlEvent.key.keysym.sym);
                    }
                } else if (sdlEvent.type == SDL_KEYUP) {
                    std::cerr << "Release: " << sdlEvent.key.keysym.sym << std::endl;
                    if (true == m_dialogActive) {
                        m_dialog->handleEvent(sdlEvent);
                    } else {
                        handleKeyRelease(sdlEvent.key.keysym.sym);
                    }
                } else if (sdlEvent.type == SDL_MOUSEBUTTONDOWN) {
                    MouseEvent::Button button;
                    switch (sdlEvent.button.button) {
                    case SDL_BUTTON_LEFT:
                        button = MouseEvent::LEFT;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        button = MouseEvent::MIDDLE;
                        break;
                    case SDL_BUTTON_RIGHT:
                        button = MouseEvent::RIGHT;
                        break;
                    default:
                        return;
                    }
                    handleMousePress(sdlEvent.button.x, sdlEvent.button.y, button);
                } else if (sdlEvent.type == SDL_MOUSEBUTTONUP) {
                    MouseEvent::Button button;
                    switch (sdlEvent.button.button) {
                    case SDL_BUTTON_LEFT:
                        button = MouseEvent::LEFT;
                        break;
                    case SDL_BUTTON_MIDDLE:
                        button = MouseEvent::MIDDLE;
                        break;
                    case SDL_BUTTON_RIGHT:
                        button = MouseEvent::RIGHT;
                        break;
                    default:
                        return;
                    }
                    handleMouseRelease(sdlEvent.button.x, sdlEvent.button.y, button);
                }
            }
        }
    }
}


void App::onDialogClose() {
    m_dialogActive = false;
}


void App::loop()
{
    const int targetFPS = 60;
    const int frameDelay = 5000 / targetFPS;
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
    Cleanup();
}


void App::update(double delta_time)
{
    if (m_dialogActive) {
        // freeze game process when dialog active
    } else {
        m_scene.update(delta_time);
    }
}


bool App::isVisible(const SDL_Rect& objectRect, std::shared_ptr<Viewport> viewport) {
    return (objectRect.x + objectRect.w > viewport->getX() &&
            objectRect.x < viewport->getX() + viewport->getWidth() &&
            objectRect.y + objectRect.h > viewport->getY() &&
            objectRect.y < viewport->getY() + viewport->getHeight());
}

void App::draw()
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 0);
    SDL_RenderClear(m_renderer);
    for (const auto& object : m_scene.getObjects()) {
        if (isVisible(object->getBoundingBox(), m_viewport)) {
            object->draw(m_renderer, m_viewport);
        }
    }
    if (m_dialogActive) {
        m_dialog->render(m_renderer);
    }
    if (m_text_texture) {
        SDL_RenderCopy(m_renderer, m_text_texture, nullptr, &m_text_rect);
    }
    SDL_RenderPresent(m_renderer);
}
