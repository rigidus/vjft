// Log.hpp

#ifndef LOG_HPP
#define LOG_HPP

#include <typeinfo>
#include <cxxabi.h>


// Макрос для функций вне классов, который выводит HEX value
#define LOG_HEX(msg, value, cnt)                                               \
    if (DBG_MSG > 0) {                                                         \
        std::cerr << ":> " << __FILE__ "::" << __FUNCTION__ << "(): ";         \
        std::cerr << msg << ": ";                                              \
        for (size_t i = 0; i < cnt; ++i) {                                     \
            unsigned char byte = (value >> (i * 8)) & 0xFF;                    \
            std::cerr << std::setw(2) << std::setfill('0')                     \
                      << std::hex << static_cast<int>(byte);                   \
        }                                                                      \
        std::cerr << std::dec << std::endl;                                    \
    }

// Макрос для функций вне классов, который выводит HEX vector
#define LOG_VEC(msg, vec)                                                      \
    if (DBG_MSG > 0) {                                                         \
        std::cerr << ":> " << __FILE__ "::" << __FUNCTION__ << "(): ";         \
        std::cerr << msg << ": [";                                             \
        for (const auto& byte : vec) {                                         \
            std::cerr << std::setw(2) << std::setfill('0') << std::hex         \
                      << static_cast<int>(byte);                               \
        }                                                                      \
        std::cerr << "]" << std::dec << std::endl;                             \
    }

// Макрос для функций вне классов
#define LOG_TXT(msg)                                                           \
    if (DBG_MSG > 0) {                                                         \
        std::cerr << ":> " << __FILE__ "::" << __FUNCTION__                    \
                  << "(): " << msg << std::endl;                               \
    }

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
#define LOG_ERR(msg)                                                           \
    if (DBG_MSG > 0) {                                                         \
        LOG_BASE("!> ", msg)                                                   \
    }
// Эти сообщения выводятся вне зависимости от DBG_MSG
#define LOG_MSG(msg)                                                           \
    std::cout << "-> " << msg << std::endl;                                    \

#endif // LOG_HPP
