// Utils.cpp
#include "Utils.hpp"

/**
   Debug print for vectors
*/
void dbg_out_vec(std::string dbgmsg, std::vector<unsigned char> vec) {
    std::cout << dbgmsg << ": ";
    for (const auto& byte : vec) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl; // Возвращаемся к десятичному формату
}

/**
   Debug print for scalars
*/
void dbg_out_scalar(std::string dbgmsg, size_t value, size_t cnt) {
    std::cout << dbgmsg << ": ";
    for (size_t i = 0; i < cnt; ++i) {
        unsigned char byte = (value >> (i * 8)) & 0xFF;
        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl; // Возвращаемся к десятичному формату
}

/**
   Split std::vector<unsigned char> to std::vector<std::vector<unsigned char>>
   by size
*/
std::vector<std::vector<unsigned char>> split_vec(
    const std::vector<unsigned char>& input, int size)
{
    std::vector<std::vector<unsigned char>> result;
    size_t length = input.size();
    for (size_t i = 0; i < length; i += size) {
        size_t currentChunkSize = std::min(size_t(size), length - i);
        std::vector<unsigned char> chunk(
            input.begin() + i, input.begin() + i + currentChunkSize);
        result.push_back(chunk);
    }
    return result;
}

/**
   Merge std::vector<std::vector<unsigned char>> to std::vector<unsigned char>
*/
std::vector<unsigned char> merge_vec(
    const std::vector<std::vector<unsigned char>>& input)
{
    std::vector<unsigned char> result;
    for (const auto& chunk : input) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    return result;
}
