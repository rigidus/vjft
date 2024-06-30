// Server.cpp
#include "Server.hpp"

Server::Server(boost::asio::io_service& io_service,
               boost::asio::io_service::strand& strand,
               const tcp::endpoint& endpoint)
    : io_service_(io_service), strand_(strand), acceptor_(io_service, endpoint)
{
    Run();
}

void Server::Run() {
    std::shared_ptr<PersonInRoom> new_participant(
        new PersonInRoom(io_service_, strand_, room_));
    acceptor_.async_accept(
        new_participant->Socket(),
        strand_.wrap(boost::bind(&Server::OnAccept, this, new_participant, _1)));
}

void Server::OnAccept(
    std::shared_ptr<PersonInRoom> new_participant,
    const boost::system::error_code& error)
{
    if (!error) {
        LOG_ERR("New participant accepted");
        new_participant->Start();
    } else {
        LOG_ERR("Error: " << error.message());
    }

    Run();
}
