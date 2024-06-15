// Message.cpp
#include "Message.hpp"

std::string Message::current_timestamp() {
    time_t t = time(0);   // get time now
    struct tm* now = localtime(&t);
    std::stringstream ss;
    ss << '[' << (now->tm_year + 1900) << '-' << std::setfill('0')
       << std::setw(2) << (now->tm_mon + 1) << '-' << std::setfill('0')
       << std::setw(2) << now->tm_mday << ' ' << std::setfill('0')
       << std::setw(2) << now->tm_hour << ":" << std::setfill('0')
       << std::setw(2) << now->tm_min << ":" << std::setfill('0')
       << std::setw(2) << now->tm_sec << "] ";
    return ss.str();
}


std::vector<std::vector<unsigned char>> Message::splitStrToVec255(const std::string& input)
{
    std::vector<std::vector<unsigned char>> result;
    const size_t chunkSize = 255;
    size_t length = input.size();
    for (size_t i = 0; i < length; i += chunkSize) {
        size_t currentChunkSize = std::min(chunkSize, length - i);
        std::vector<unsigned char> chunk(input.begin() + i, input.begin() + i +
                                         currentChunkSize);
        result.push_back(chunk);
    }
    return result;
}

std::string Message::joinVec255ToStr(const std::vector<std::vector<unsigned char>>& input) {
    std::string result;
    for (const auto& chunk : input) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    return result;
}


std::vector<unsigned char> Message::packMsgStruct(const MsgStruct& msgStruct) {
    std::vector<unsigned char> buffer;

    // crc
    buffer.insert(buffer.end(), msgStruct.crc.begin(), msgStruct.crc.end());

    // siz
    unsigned char sizBytes[sizeof(msgStruct.siz)];
    std::memcpy(sizBytes, &msgStruct.siz, sizeof(msgStruct.siz));
    buffer.insert(buffer.end(), sizBytes, sizBytes + sizeof(msgStruct.siz));

    // msg
    buffer.insert(buffer.end(), msgStruct.msg.begin(), msgStruct.msg.end());

    return buffer;
}

std::vector<unsigned char> Message::packEnvelope(const Envelope& envelope) {
    std::vector<unsigned char> packed;

    // Упаковываем длину (len)
    packed.push_back(static_cast<unsigned char>(envelope.len >> 8)); // старший байт
    packed.push_back(static_cast<unsigned char>(envelope.len & 0xFF)); // младший байт

    // Упаковываем зашифрованное сообщение (msg)
    packed.insert(packed.end(), envelope.msg.begin(), envelope.msg.end());

    // Упаковываем зашифрованную подпись (sig)
    packed.insert(packed.end(), envelope.sig.begin(), envelope.sig.end());

    return packed;
}

std::optional<Envelope> Message::unpackEnvelope(const std::vector<unsigned char>& packed) {
    // Проверка минимального размера пакета
    if (packed.size() < 2 + 512 + 1024) {
        std::cerr << "Error: Packed data is too small to contain an Envelope" << std::endl;
        return std::nullopt;
    }

    Envelope envelope;

    // Извлечение длины (len)
    envelope.len =
        (static_cast<uint16_t>(packed[0]) << 8) | static_cast<uint16_t>(packed[1]);

    // Проверка корректности длины сообщения
    if (packed.size() != 2 + envelope.len + 1024) {
        std::cerr << "Error: Packed data length does not match the length specified in the Envelope" << std::endl;
        return std::nullopt;
    }

    // Извлечение зашифрованного сообщения (msg)
    envelope.msg =
        std::vector<unsigned char>(packed.begin() + 2, packed.begin() + 2 + envelope.len);

    // Извлечение зашифрованной подписи (sig)
    envelope.sig =
        std::vector<unsigned char>(packed.begin() + 2 + envelope.len, packed.end());

    return envelope;
}

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
