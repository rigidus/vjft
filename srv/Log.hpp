// Log.hpp

#ifndef LOG_HPP
#define LOG_HPP

#include <typeinfo>
#include <cxxabi.h>

// Макрос для функций вне классов, который выводит HEX value
#define LOG_HEX(msg, value, cnt)                                              \
    {                                                                         \
        std::cout << ":> " << __FILE__ "::" << __FUNCTION__ << "(): ";        \
        std::cout << msg << ": ";                                             \
        for (size_t i = 0; i < cnt; ++i) {                                    \
            unsigned char byte = (value >> (i * 8)) & 0xFF;                   \
            std::cout << std::setw(2) << std::setfill('0')                    \
                      << std::hex << static_cast<int>(byte);                  \
        }                                                                     \
        std::cout << std::dec << std::endl;                                   \
    }

// Макрос для функций вне классов, который выводит HEX vector
#define LOG_VEC(msg, vec)                                                     \
    {                                                                         \
        std::cout << ":> " << __FILE__ "::" << __FUNCTION__ << "(): ";        \
        std::cout << msg << ": [";                                            \
        for (const auto& byte : vec) {                                        \
            std::cout << std::setw(2) << std::setfill('0') << std::hex        \
                      << static_cast<int>(byte);                              \
        }                                                                     \
        std::cout << "]" << std::dec << std::endl;                            \
    }

// Макрос для функций вне классов
#define LOG_TXT(msg) std::cerr << ":> " << __FILE__ "::" << __FUNCTION__      \
    << "(): " << msg << std::endl

// Макрос для деманглинга имени класса
#define DEMANGLE(type) abi::__cxa_demangle(typeid(type).name(), 0, 0, &status)

// Макрос для логирования с использованием деманглинга
#define LOG_BASE(level, msg)                                                   \
    {                                                                          \
        int status;                                                            \
        char* demangled = DEMANGLE(*this);                                     \
        std::cerr << level << (status == 0 ? demangled : typeid(*this).name()) \
                  << "::" << __FUNCTION__ << "(): "                            \
                  << msg << std::endl;                                         \
        if (demangled) free(demangled);                                        \
    }

// Макросы для сообщений об ошибках и информационных сообщений
#define LOG_ERR(msg) LOG_BASE("!> ", msg)
#define LOG_MSG(msg) LOG_BASE("-> ", msg)

#endif // LOG_HPP
