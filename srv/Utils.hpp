// Utils.hpp
#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

/**
   Debug print for vectors
*/
void dbg_out_vec(std::string dbgmsg, std::vector<unsigned char> vec);

/**
   Debug print for scalars
*/
void dbg_out_scalar(std::string dbgmsg, size_t value, size_t cnt);

/**
   Split std::vector<unsigned char> to std::vector<std::vector<unsigned char>>
   by size
*/
std::vector<std::vector<unsigned char>> split_vec(
    const std::vector<unsigned char>& input, int size);

/**
   Merge std::vector<std::vector<unsigned char>> to std::vector<unsigned char>
*/
    std::vector<unsigned char> merge_vec(
        const std::vector<std::vector<unsigned char>>& input);

#endif // UTILS
