#include "ChatRoom.hpp"

void ChatRoom::Enter(
    std::shared_ptr<Participant> participant, const std::string& nickname)
{
    LOG_MSG("Participant entered with nickname: " << nickname);
    participants_.insert(participant);
    name_table_[participant] = nickname;
    std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
                  boost::bind(&Participant::OnMessage, participant, _1));
    LOG_MSG("Participant added. Total participants: " << participants_.size());
}

void ChatRoom::Leave(std::shared_ptr<Participant> participant) {
    LOG_MSG("Participant leaving");
    participants_.erase(participant);
    name_table_.erase(participant);
    LOG_MSG("Participant removed. Total participants: " << participants_.size());
}

void ChatRoom::Broadcast(const std::vector<unsigned char>& msg,
                         std::shared_ptr<Participant> participant)
{
    std::string dbgstr(msg.begin(), msg.end());
    LOG_VEC("Broadcasting message", msg);

    uint16_t msg_len = static_cast<uint16_t>(msg.size());
    char len_bytes[2];
    len_bytes[0] = static_cast<unsigned char>(msg_len & 0xFF); // младший байт
    len_bytes[1] = static_cast<unsigned char>((msg_len >> 8) & 0xFF); // старший

    // Создаем новый вектор для сообщения с длиной в начале
    std::vector<unsigned char> bcast;
    bcast.insert(bcast.end(), len_bytes, len_bytes + 2);
    bcast.insert(bcast.end(), msg.begin(), msg.end());

    // Debug print
    LOG_ERR("bcast size:" << msg_len);
    LOG_HEX("bcast size in hex", msg_len, 2);
    LOG_VEC("bcast", bcast);

    // Добавление сообщения в историю
    recent_msgs_.push_back(bcast);
    while (recent_msgs_.size() > max_recent_msgs) {
        recent_msgs_.pop_front();
    }

    LOG_MSG("Broadcasting to " << participants_.size() << " participants");

    // Рассылка сообщения всем участникам
    std::for_each(participants_.begin(), participants_.end(),
                  boost::bind(&Participant::OnMessage, _1, std::ref(bcast)));
}

std::string ChatRoom::GetNickname(std::shared_ptr<Participant> participant) {
    return name_table_[participant];
}
