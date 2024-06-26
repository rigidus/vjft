// PersonInRoom.hpp
#ifndef PERSONINROOM_HPP
#define PERSONINROOM_HPP

#include <deque>
#include <array>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <algorithm>
#include "ChatRoom.hpp"

using boost::asio::ip::tcp;

class PersonInRoom : public Participant, public std::enable_shared_from_this<PersonInRoom>
{
public:
    PersonInRoom(boost::asio::io_service& io_service,
                 boost::asio::io_service::strand& strand, ChatRoom& room);
    tcp::socket& Socket();
    void Start();
    void OnMessage(const std::vector<char>& msg);

private:
    void HeaderHandler(const boost::system::error_code& error);
    void ReadHandler(const boost::system::error_code& error);
    void WriteHandler(const boost::system::error_code& error);

    tcp::socket socket_;
    boost::asio::io_service::strand& strand_;
    ChatRoom& room_;
    std::array<char, MAX_NICKNAME> nickname_;
    std::vector<char> read_msg_;
    std::deque<std::vector<char>> write_msgs_;
};

#endif // PERSONINROOM_HPP
