#include "ChatRoom.hpp"

void ChatRoom::Enter(
    std::shared_ptr<Participant> participant, const std::string& nickname)
{
    std::cout << "ChatRoom::Enter: Participant entered with nickname: "
              << nickname << std::endl;
    participants_.insert(participant);
    name_table_[participant] = nickname;
    std::for_each(recent_msgs_.begin(), recent_msgs_.end(),
                  boost::bind(&Participant::OnMessage, participant, _1));
    std::cout << "ChatRoom::Enter: Participant added. Total participants: "
              << participants_.size() << std::endl;
}

void ChatRoom::Leave(std::shared_ptr<Participant> participant) {
    std::cout << "ChatRoom::Leave: Participant leaving" << std::endl;
    participants_.erase(participant);
    name_table_.erase(participant);
    std::cout << "ChatRoom::Leave: Participant removed. Total participants: "
              << participants_.size() << std::endl;
}



void ChatRoom::Broadcast(const std::vector<char>& msg,
                         std::shared_ptr<Participant> participant)
{
    std::string dbgstr(msg.begin(), msg.end());
    std::cout << "\n---ChatRoom::Broadcast(): Broadcasting message: "
              << dbgstr
              << std::endl;

    uint16_t msg_len = static_cast<uint16_t>(msg.size());
    char len_bytes[2];
    len_bytes[0] = static_cast<char>(msg_len & 0xFF); // младший байт
    len_bytes[1] = static_cast<char>((msg_len >> 8) & 0xFF); // старший

    // Создаем новый вектор для сообщения с длиной в начале
    std::vector<char> bcast;
    bcast.insert(bcast.end(), len_bytes, len_bytes + 2);
    bcast.insert(bcast.end(), msg.begin(), msg.end());

    std::cout << "ChatRoom::Broadcast(): size: " << msg.size() << std::endl;

    // Добавление сообщения в историю
    recent_msgs_.push_back(bcast);
    while (recent_msgs_.size() > max_recent_msgs) {
        recent_msgs_.pop_front();
    }

    std::cout << "ChatRoom::Broadcast(): Broadcasting to "
              << participants_.size() << " participants" << std::endl;

    // Рассылка сообщения всем участникам
    std::for_each(participants_.begin(), participants_.end(),
                  boost::bind(&Participant::OnMessage, _1, std::ref(bcast)));
}

std::string ChatRoom::GetNickname(std::shared_ptr<Participant> participant) {
    return name_table_[participant];
}
