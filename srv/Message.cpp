// Message.cpp
#include "Message.hpp"

std::string Message::pack(const std::map<std::string, std::string>& data) {
    std::stringstream ss;
    for (const auto& pair : data) {
        ss << "(:" << pair.first << " \"" << pair.second << "\") ";
    }
    return ss.str();
}

std::map<std::string, std::string> Message::unpack(const std::string& packed_message) {
    std::map<std::string, std::string> data;
    size_t pos = 0;

    while ((pos = packed_message.find("(:", pos)) != std::string::npos) {
        size_t key_start = pos + 2;
        size_t key_end = packed_message.find(" \"", key_start);
        if (key_end == std::string::npos) {
            break;
        }
        std::string key = packed_message.substr(key_start, key_end - key_start);

        size_t value_start = key_end + 2;
        size_t value_end = packed_message.find("\")", value_start);
        if (value_end == std::string::npos) {
            break;
        }
        std::string value = packed_message.substr(value_start, value_end - value_start);

        data[key] = value;
        pos = value_end + 2;
    }

    return data;
}
