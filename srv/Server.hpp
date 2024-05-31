// Server.hpp
#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "PersonInRoom.hpp"
#include "ChatRoom.hpp"

class Server {
public:
    Server(boost::asio::io_service& io_service,
           boost::asio::io_service::strand& strand,
           const tcp::endpoint& endpoint);

private:
    void Run();
    void OnAccept(std::shared_ptr<PersonInRoom> new_participant, const boost::system::error_code& error);

    boost::asio::io_service& io_service_;
    boost::asio::io_service::strand& strand_;
    tcp::acceptor acceptor_;
    ChatRoom room_;
};

#endif // SERVER_HPP
