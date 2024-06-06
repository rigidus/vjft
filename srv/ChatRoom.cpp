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


// void ChatRoom::Broadcast(std::array<char, MAX_IP_PACK_SIZE>& msg, std::shared_ptr<Participant> participant) {
//     time_t t = time(0);   // get time now
//     struct tm* now = localtime(&t);
//     std::stringstream ss;
//     ss << '[' << (now->tm_year + 1900) << '-' << std::setfill('0')
//        << std::setw(2) << (now->tm_mon + 1) << '-' << std::setfill('0')
//        << std::setw(2) << now->tm_mday << ' ' << std::setfill('0')
//        << std::setw(2) << now->tm_hour << ":" << std::setfill('0')
//        << std::setw(2) << now->tm_min << ":" << std::setfill('0')
//        << std::setw(2) << now->tm_sec << "] ";
//     std::string timestamp = ss.str();
//     std::string nickname = GetNickname(participant);
//     std::array<char, MAX_IP_PACK_SIZE> formatted_msg;

//     strcpy(formatted_msg.data(), timestamp.c_str());
//     strcat(formatted_msg.data(), nickname.c_str());
//     strcat(formatted_msg.data(), msg.data());

//     recent_msgs_.push_back(formatted_msg);
//     while (recent_msgs_.size() > max_recent_msgs) {
//         recent_msgs_.pop_front();
//     }

//     std::for_each(participants_.begin(), participants_.end(),
//                   boost::bind(&Participant::OnMessage, _1, std::ref(formatted_msg)));
// }

void ChatRoom::Broadcast(std::array<char, MAX_IP_PACK_SIZE>& msg, std::shared_ptr<Participant> participant) {
    // time_t t = time(0);   // get time now
    // struct tm* now = localtime(&t);
    // std::stringstream ss;
    // ss << '[' << (now->tm_year + 1900) << '-' << std::setfill('0')
    //    << std::setw(2) << (now->tm_mon + 1) << '-' << std::setfill('0')
    //    << std::setw(2) << now->tm_mday << ' ' << std::setfill('0')
    //    << std::setw(2) << now->tm_hour << ":" << std::setfill('0')
    //    << std::setw(2) << now->tm_min << ":" << std::setfill('0')
    //    << std::setw(2) << now->tm_sec << "] ";
    // std::string timestamp = ss.str();
    // std::string nickname = GetNickname(participant);

    // std::string to = "recipient";
    // std::string checksum = "checksum_value";
    // bool encrypted = false;
    std::array<char, MAX_IP_PACK_SIZE> formatted_msg;
    strcpy(formatted_msg.data(),
           // (timestamp + "to:" + to + "|checksum:" + checksum + "|encrypted:" + (encrypted ? "true" : "false") + "|message:" + msg.data()).c_str()
           msg.data()
        );

    recent_msgs_.push_back(formatted_msg);
    while (recent_msgs_.size() > max_recent_msgs) {
        recent_msgs_.pop_front();
    }

    std::for_each(participants_.begin(), participants_.end(),
                  boost::bind(&Participant::OnMessage, _1, std::ref(formatted_msg)));
}


std::string ChatRoom::GetNickname(std::shared_ptr<Participant> participant) {
    return name_table_[participant];
}
