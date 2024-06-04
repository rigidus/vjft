// TextField.cpp
#include <SDL2/SDL_clipboard.h>
#include "TextField.hpp"

TextField::TextField(int x, int y, int w, int h, TTF_Font* font, SDL_Color textColor, int maxLines)
    : x(x), y(y), width(w), height(h), font(font), textColor(textColor), maxLines(maxLines), focused(false) {

    // Отладочный вывод
    std::cout << "TextField created with parameters:" << std::endl;
    std::cout << "  Position: { x: " << x << ", y: " << y << " }" << std::endl;
    std::cout << "  Size: { width: " << width << ", height: " << height << " }" << std::endl;
    std::cout << "  Font: " << font << std::endl;
    std::cout << "  Text color: { r: " << (int)textColor.r << ", g: " << (int)textColor.g << ", b: " << (int)textColor.b << ", a: " << (int)textColor.a << " }" << std::endl;
    std::cout << "  Max lines: " << maxLines << std::endl;
    std::cout << "  Focused: " << (focused ? "true" : "false") << std::endl;
}


std::vector<std::string> TextField::splitString(const std::string& input) {
    std::vector<std::string> result;
    std::string current;

    for (char ch : input) {
        if (std::isspace(ch)) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
            result.push_back(std::string(1, ch));
        } else {
            current += ch;
        }
    }

    // Добавляем последнее слово, если оно есть
    if (!current.empty()) {
        result.push_back(current);
    }

    return result;
}


std::vector<std::string> TextField::wrapSplitted(const std::vector<std::string>& input) {
    std::vector<std::string> result;
    std::string curr;

    for (const auto& element : input) {
        if (element == "\n") {
            curr += element;
            result.push_back(curr);
            curr.clear();
        } else {
            std::string candidat = curr + element;
            int text_width = 0;
            TTF_SizeText(font, candidat.c_str(), &text_width, nullptr);
            if (text_width < width) {
                curr = candidat;
            } else {
                if (!curr.empty()) {
                    result.push_back(curr);
                }
                curr = element;
            }
        }
    }

    // Добавляем остаток строки, если он есть
    if (!curr.empty()) {
        result.push_back(curr);
    }

    return result;
}


std::string joinStrings(const std::list<std::string>& lst, const std::string& delimiter) {
    std::ostringstream oss;
    for (auto it = lst.begin(); it != lst.end(); ++it) {
        if (it != lst.begin()) {
            oss << delimiter;
        }
        oss << *it;
    }
    return oss.str();
}


void TextField::handleEvent(SDL_Event& e) {
    if (!focused) return;
    if (e.type == SDL_TEXTINPUT) {
        std::cerr << "----> Textnput=\"" << e.text.text << "\"" << std::endl;

        // Join-им строки
        std::string contents = joinStrings(lines, "");

        // Добавляем введенную букву к строке
        contents += e.text.text;

        std::vector<std::string> wrapped = wrapSplitted(splitString(contents));
        lines.assign(wrapped.begin(), wrapped.end());



        // // Если строк нет, добавляем пустую строку
        // if (lines.empty()) {
        //     lines.push_back("");
        // }

        // // Сохраняем текущее состяние для возможного отката
        // std::list<std::string> linesBackup = lines;


        // // Отладочный Вывод lines
        // for (const auto& line : lines) {
        //     std::cout << ". " << line << std::endl;
        // }

        // // Получаем ширину последней строки
        // int lastStrWidth;
        // TTF_SizeText(font, lines.back().c_str(), &lastStrWidth, nullptr);
        // std::cerr << "-> SizeText=" << lastStrWidth << std::endl;

        // // Если ширина последней строки превышает width
        // if (lastStrWidth > width) {
        //     std::cerr << "-> !width!" << std::endl;
        //     // Получаем ширину пробела в spaceWidth
        //     int spaceWidth;
        //     TTF_SizeText(font, " ", &spaceWidth, nullptr);

        //     // Разбиваем строку на слова
        //     std::vector<std::string> words;
        //     std::istringstream iss(lines.back());
        //     std::string word;
        //     while (iss >> word) {
        //         std::cerr << "-> !word!=\"" << word << "\"" << std::endl;
        //         words.push_back(word);
        //     }

        //     // Проходим по словам и добавляем их к oss,
        //     // пока ширина не превышает width, как только (если) это случится
        //     // записываем последний элемент вектора и добавляем новый,
        //     // содержащий оставшиеся слова
        //     std::ostringstream oss;
        //     int accWidth = 0;
        //     size_t wordIndex = 0;
        //     for (; wordIndex < words.size(); ++wordIndex) {
        //         const auto& word = words[wordIndex];
        //         int wordWidth;
        //         TTF_SizeText(font, word.c_str(), &wordWidth, nullptr);
        //         // Если накопленная ширина + ширина слова + пробел
        //         // уже больше чем width, та выходим из цикла
        //         if (accWidth + wordWidth + spaceWidth > width) {
        //             break;
        //         }
        //         // Накапливаем ширину
        //         accWidth += wordWidth + spaceWidth;
        //         // Добавляем слово и пробел к oss
        //         oss << word << ' ';
        //     }
        //     // Иначе переписываем накопленную строку в newLine
        //     std::string newLine = oss.str();
        //     // И удаляем из нее последний добавленный пробел
        //     if (!newLine.empty() && newLine.back() == ' ') {
        //         newLine.pop_back();
        //     }
        //     // Записываем newLine строку в последний элемент вектора
        //     lines.back() = newLine;

        //     // Очищаем накопленную строку oss
        //     oss.str("");
        //     oss.clear();
        //     // Добавляем оставшиеся слова в накопленную строку oss
        //     for (++wordIndex; wordIndex < words.size(); ++wordIndex) {
        //         oss << words[wordIndex] << ' ';
        //     }
        //     // Удаляем последний пробел и переписываем в remainingWords
        //     std::string remainingWords = oss.str();
        //     if (!remainingWords.empty() && remainingWords.back() == ' ') {
        //         remainingWords.pop_back();
        //     }
        //     // Добавляем remainingWords в свежесозданный последний элемент вектора
        //     if (!remainingWords.empty()) {
        //         lines.push_back(remainingWords);
        //     }

        //     // Проверяем, что количество строк не превышает максимальное
        //     if (lines.size() >= maxLines) {
        //         // Если это так - откатываем изменения
        //         Utils::Bell();
        //         lines = linesBackup;
        //         return;
        //     }

        // }
    } else if (e.type == SDL_KEYDOWN) {
        std::cerr << "-> TextField::handleEvent KEY_DOWN [6] " << e.key.keysym.sym << std::endl;
        switch (e.key.keysym.sym) {
        case SDLK_BACKSPACE:
            if (!lines.empty() && !lines.back().empty()) {
                lines.back().pop_back();
                if (lines.back().empty() && lines.size() > 1) {
                    lines.pop_back();
                }
            } else if (!lines.empty()) {
                lines.pop_back();
            }
            break;
    //     case SDLK_v:
    //         if (SDL_GetModState() & KMOD_CTRL) {
    //             char* clipboardText = SDL_GetClipboardText();
    //             if (clipboardText) {
    //                 lines.back() += clipboardText;
    //                 SDL_free(clipboardText);
    //                 wrapLine();
    //             }
    //         }
    //         break;
    //     case SDLK_c:
    //         if (SDL_GetModState() & KMOD_CTRL) {
    //             SDL_SetClipboardText(lines.back().c_str());
    //         }
    //         break;
        case SDLK_ESCAPE:
            loseFocus();
            break;
        default:
            break;
        }
    }
}

void TextField::render(SDL_Renderer* renderer) {
    // std::cerr << "-> TextField::Render [10]" << std::endl;
    int offsetY = 0;
    for (const auto& line : lines) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, line.c_str(), textColor);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_Rect renderQuad = { x, y + offsetY, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, nullptr, &renderQuad);
        offsetY += surface->h;
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void TextField::setFocus() {
    std::cerr << "-> TextField setFocus StartTextInput [3]" << std::endl;
    focused = true;
    SDL_StartTextInput();
}

void TextField::loseFocus() {
    std::cerr << "-> TextField loseFocus StopTextInput [3]" << std::endl;
    focused = false;
    SDL_StopTextInput();
}

bool TextField::hasFocus() const {
    return focused;
}

void TextField::setInitialLine(const std::string& line) {
    lines.clear();
    lines.push_back(line);
}
