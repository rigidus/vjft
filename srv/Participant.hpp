// Participant.hpp
#ifndef PARTICIPANT_HPP
#define PARTICIPANT_HPP

#include <array>
#include "Protocol.hpp"

class Participant {
public:
    virtual ~Participant() {}
    virtual void OnMessage(const std::vector<char>& msg) = 0;
};

#endif // PARTICIPANT_HPP
