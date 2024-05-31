// Participant.hpp
#ifndef PARTICIPANT_HPP
#define PARTICIPANT_HPP

#include <array>
#include "Protocol.hpp"

class Participant {
public:
    virtual ~Participant() {}
    virtual void OnMessage(std::array<char, MAX_IP_PACK_SIZE>& msg) = 0;
};

#endif // PARTICIPANT_HPP
