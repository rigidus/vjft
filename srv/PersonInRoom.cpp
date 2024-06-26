#include "PersonInRoom.hpp"

PersonInRoom::PersonInRoom(boost::asio::io_service& io_service,
                           boost::asio::io_service::strand& strand, ChatRoom& room)
    : socket_(io_service), strand_(strand), room_(room), read_msg_(2)
{
    // начинаем с буфера размером 2 байта для заголовка
}

tcp::socket& PersonInRoom::Socket() {
    return socket_;
}

void PersonInRoom::Start() {
    std::cout << "PersonInRoom::Start(): Participant starting" << std::endl;
    // читаем сначала 2 байта заголовка
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), 2),
                            strand_.wrap(boost::bind(&PersonInRoom::HeaderHandler,
                                                     shared_from_this(), _1)));
    std::cout << "PersonInRoom::Start(): async_read initiated" << std::endl;
    room_.Enter(shared_from_this(), "ParticipantNickname");
}

void PersonInRoom::HeaderHandler(const boost::system::error_code& error) {
    if (!error) {
        uint16_t msg_length =
            (static_cast<uint16_t>(read_msg_[1]) << 8) |
            static_cast<uint16_t>(read_msg_[0]);

        std::cout << "PersonInRoom::HeaderHandler(): msg_length: " << msg_length
                  << std::endl;

        read_msg_.resize(msg_length);
        // читаем само сообщение
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), msg_length),
                                strand_.wrap(boost::bind(&PersonInRoom::ReadHandler,
                                                         shared_from_this(), _1)));
    } else {
        room_.Leave(shared_from_this());
    }
}

void PersonInRoom::OnMessage(const std::vector<char>& msg) {
    std::cout << "PersonInRoom::OnMessage(): Adding message to queue" << std::endl;
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    std::cout << "PersonInRoom::OnMessage(): Added message to write queue, size: "
              << msg.size() << std::endl;
    if (!write_in_progress) {
        std::cout << "PersonInRoom::OnMessage(): Starting async_write" << std::endl;
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front().data(),
                                                     write_msgs_.front().size()),
                                 strand_.wrap(boost::bind(&PersonInRoom::WriteHandler,
                                                          shared_from_this(), _1)));
    }
}

void PersonInRoom::ReadHandler(const boost::system::error_code& error) {
    if (!error) {
        std::vector<char> received_msg(read_msg_.begin(), read_msg_.end());
        std::cout << "PersonInRoom::ReadHandler(): Server received message (size: "
                  << received_msg.size() << "): "
                  << std::string(received_msg.begin(), received_msg.end())
                  << std::endl;

        room_.Broadcast(received_msg, shared_from_this());

        std::cout << "PersonInRoom::ReadHandler(): Starting async_read for next message"
                  << std::endl;

        // готовим буфер для чтения следующего заголовка
        read_msg_.resize(2);
        // снова читаем заголовок
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), 2),
                                strand_.wrap(boost::bind(&PersonInRoom::HeaderHandler,
                                                         shared_from_this(), _1)));
    } else {
        room_.Leave(shared_from_this());
    }
}

void PersonInRoom::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        std::cout << "PersonInRoom::WriteHandler(): Message written successfully"
                  << std::endl;

        write_msgs_.pop_front();

        if (!write_msgs_.empty()) {
            std::cout << "PersonInRoom::WriteHandler(): "
                      << "Starting async_write for next message" << std::endl;

            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front().data(),
                                                         write_msgs_.front().size()),
                                     strand_.wrap(boost::bind(&PersonInRoom::WriteHandler,
                                                              shared_from_this(), _1)));
        }
    } else {
        std::cerr << "PersonInRoom::WriteHandler(): Error: " << error.message()
                  << std::endl;
        room_.Leave(shared_from_this());
    }
}

// void PersonInRoom::NicknameHandler(const boost::system::error_code& error) {
//     if (!error) {
//         // Проверка длины никнейма и добавление двоеточия и пробела в конце
//         if (strlen(nickname_.data()) <= MAX_NICKNAME - 2) {
//             strcat(nickname_.data(), ": ");
//         } else {
//             nickname_[MAX_NICKNAME - 2] = ':';
//             nickname_[MAX_NICKNAME - 1] = ' ';
//         }

//         // Вывод отладочного сообщения о полученном никнейме
//         std::cout << "Received nickname: " << nickname_.data() << std::endl;

//         // Добавление участника в комнату чата
//         room_.Enter(shared_from_this(), std::string(nickname_.data()));

//         // Инициализация асинхронного чтения следующего сообщения
//         boost::asio::async_read(socket_,
//                                 boost::asio::buffer(read_msg_.data(), read_msg_.size()),
//                                 strand_.wrap(boost::bind(&PersonInRoom::ReadHandler, shared_from_this(), _1)));
//     } else {
//         room_.Leave(shared_from_this());
//     }
// }
// void PersonInRoom::NicknameHandler(const boost::system::error_code& error) {
//     if (!error) {
//         room_.Enter(shared_from_this(), std::string(nickname_.data()));
//         boost::asio::async_read(socket_,
//                                 boost::asio::buffer(read_msg_.data(), read_msg_.size()),
//                                 strand_.wrap(boost::bind(&PersonInRoom::ReadHandler,
//                                                          shared_from_this(), _1)));
//     } else {
//         room_.Leave(shared_from_this());
//     }
// }
