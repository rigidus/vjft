// ChatRoom.cpp
#include "ChatRoom.hpp"

void ChatRoom::Enter(std::shared_ptr<Participant> participant, const std::string& nickname) {
    participants_.insert(participant);
    name_table_[participant] = nickname;
    std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
                  boost::bind(&Participant::OnMessage, participant, _1));
}

void ChatRoom::Leave(std::shared_ptr<Participant> participant) {
    participants_.erase(participant);
    name_table_.erase(participant);
}

void ChatRoom::Broadcast(std::array<char, MAX_IP_PACK_SIZE>& msg, std::shared_ptr<Participant> participant) {
    // std::string nickname = GetNickname(participant);
    std::array<char, MAX_IP_PACK_SIZE> formatted_msg = msg;

    // Добавление сообщения в историю
    recent_msgs_.push_back(formatted_msg);
    while (recent_msgs_.size() > max_recent_msgs) {
        recent_msgs_.pop_front();
    }

    // Рассылка сообщения всем участникам
    std::for_each(participants_.begin(), participants_.end(),
                  boost::bind(&Participant::OnMessage, _1, std::ref(formatted_msg)));
}

std::string ChatRoom::GetNickname(std::shared_ptr<Participant> participant) {
    return name_table_[participant];
}
