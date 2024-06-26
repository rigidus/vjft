#ifndef CHATROOM_HPP
#define CHATROOM_HPP

#include <string>
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

class ChatRoom {
public:
    void Enter(std::shared_ptr<Participant> participant, const std::string& nickname);
    void Leave(std::shared_ptr<Participant> participant);
    void Broadcast(const std::vector<char>& msg, std::shared_ptr<Participant> participant);
    std::string GetNickname(std::shared_ptr<Participant> participant);

private:
    enum { max_recent_msgs = 100 };
    std::unordered_set<std::shared_ptr<Participant>> participants_;
    std::unordered_map<std::shared_ptr<Participant>, std::string> name_table_;
    std::deque<std::vector<char>> recent_msgs_;
};

#endif // CHATROOM_HPP
