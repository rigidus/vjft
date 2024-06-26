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

class Message {

public:
    static std::string pack(const std::map<std::string, std::string>& data);
    static std::map<std::string, std::string> unpack(const std::string& packed_message);
private:

};

#endif // MESSAGE_HPP
