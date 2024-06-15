#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <array>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <map>
#include <Client.hpp>
#include "Message.hpp"
#include "defs.hpp"

struct MsgStruct {
    std::array<unsigned char, HASH_SIZE>     crc;  // 32
    uint16_t                                 siz;  // 2
    std::string                              msg;
};

struct Envelope {
    uint16_t len;                   // 2
    std::vector<unsigned char> msg; // 512
    std::vector<unsigned char> sig; // 1024
};


class Message {

public:
    static std::vector<std::vector<unsigned char>> splitStrToVec255(
        const std::string& input);
    static std::string joinVec255ToStr(
        const std::vector<std::vector<unsigned char>>& input);
    static std::vector<unsigned char> packMsgStruct(const MsgStruct& msgStruct);
    static std::vector<unsigned char> packEnvelope(const Envelope& envelope);
    static std::optional<Envelope> unpackEnvelope(
        const std::vector<unsigned char>& packed);
    static std::string pack(const std::map<std::string, std::string>& data);
    static std::map<std::string, std::string> unpack(const std::string& packed_message);

private:
    static std::string current_timestamp();
};

#endif // MESSAGE_HPP
