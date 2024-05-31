// PersonInRoom.cpp
#include "PersonInRoom.hpp"

PersonInRoom::PersonInRoom(boost::asio::io_service& io_service,
                           boost::asio::io_service::strand& strand, ChatRoom& room)
    : socket_(io_service), strand_(strand), room_(room) {
}

tcp::socket& PersonInRoom::Socket() {
    return socket_;
}

void PersonInRoom::Start() {
    boost::asio::async_read(socket_,
                            boost::asio::buffer(nickname_, nickname_.size()),
                            strand_.wrap(boost::bind(&PersonInRoom::NicknameHandler, shared_from_this(), _1)));
}

void PersonInRoom::OnMessage(std::array<char, MAX_IP_PACK_SIZE>& msg) {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress) {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                                 strand_.wrap(boost::bind(&PersonInRoom::WriteHandler, shared_from_this(), _1)));
    }
}

void PersonInRoom::NicknameHandler(const boost::system::error_code& error) {
    if (strlen(nickname_.data()) <= MAX_NICKNAME - 2) {
        strcat(nickname_.data(), ": ");
    } else {
        nickname_[MAX_NICKNAME - 2] = ':';
        nickname_[MAX_NICKNAME - 1] = ' ';
    }

    room_.Enter(shared_from_this(), std::string(nickname_.data()));

    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_, read_msg_.size()),
                            strand_.wrap(boost::bind(&PersonInRoom::ReadHandler, shared_from_this(), _1)));
}

void PersonInRoom::ReadHandler(const boost::system::error_code& error) {
    if (!error) {
        room_.Broadcast(read_msg_, shared_from_this());

        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_, read_msg_.size()),
                                strand_.wrap(boost::bind(&PersonInRoom::ReadHandler, shared_from_this(), _1)));
    } else {
        room_.Leave(shared_from_this());
    }
}

void PersonInRoom::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        write_msgs_.pop_front();

        if (!write_msgs_.empty()) {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                                     strand_.wrap(boost::bind(&PersonInRoom::WriteHandler, shared_from_this(), _1)));
        }
    } else {
        room_.Leave(shared_from_this());
    }
}
