// ChatRoom.hpp
#ifndef CHATROOM_HPP
#define CHATROOM_HPP

#include <string>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <boost/bind.hpp>
#include <algorithm>
#include "Participant.hpp"
#include "Protocol.hpp"
#include "Utils.hpp"
#include "Message.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>


class ChatRoom {
public:
    void Enter(std::shared_ptr<Participant> participant, const std::string& nickname);
    void Leave(std::shared_ptr<Participant> participant);
    void Broadcast(std::array<char, MAX_IP_PACK_SIZE>& msg, std::shared_ptr<Participant> participant);
    std::string GetNickname(std::shared_ptr<Participant> participant);

private:
    enum { max_recent_msgs = 100 };
    std::unordered_set<std::shared_ptr<Participant>> participants_;
    std::unordered_map<std::shared_ptr<Participant>, std::string> name_table_;
    std::deque<std::array<char, MAX_IP_PACK_SIZE>> recent_msgs_;
};

#endif // CHATROOM_HPP
