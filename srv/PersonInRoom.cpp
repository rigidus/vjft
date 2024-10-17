#include "PersonInRoom.hpp"

PersonInRoom::PersonInRoom(boost::asio::io_service& io_service,
                           boost::asio::io_service::strand& strand, ChatRoom& room)
    : socket_(io_service),
      strand_(strand),
      room_(room),
      read_msg_(2),
      deadline_(io_service)
{
    // начинаем с буфера размером 2 байта для заголовка
    // и неустановленного таймера
    deadline_.expires_at(boost::asio::steady_timer::time_point::max());
}

void PersonInRoom::CheckDeadline() {
    if (deadline_.expiry() <= boost::asio::steady_timer::clock_type::now()) {
        // Тайм-аут произошел, закрываем соединение
        socket_.close();
        deadline_.expires_at(boost::asio::steady_timer::time_point::max());
    }

    deadline_.async_wait(boost::bind(&PersonInRoom::CheckDeadline, shared_from_this()));
}

tcp::socket& PersonInRoom::Socket() {
    return socket_;
}

void PersonInRoom::Start() {
    LOG_ERR("Participant starting");

    // Запускаем проверку тайм-аутов после создания объекта
    CheckDeadline();

    // читаем сначала 2 байта заголовка
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.data(), 2),
        strand_.wrap(boost::bind(&PersonInRoom::HeaderHandler,
                                 shared_from_this(), _1)));
    LOG_ERR("async_read initiated");
    room_.Enter(shared_from_this(), "ParticipantNickname"); // TODO: nickname
}

void PersonInRoom::HeaderHandler(const boost::system::error_code& error) {
    if (!error) {
        uint16_t msg_length =
            (static_cast<uint16_t>(read_msg_[1]) << 8) |
            static_cast<uint16_t>(read_msg_[0]);

        LOG_ERR("msg_length: " << msg_length);

        if (msg_length > MAX_PACK_SIZE) {
            LOG_ERR("Error: Message length exceeds maximum allowed size: "
                    << msg_length);
            room_.Leave(shared_from_this());
            return;
        }

        read_msg_.resize(msg_length);

        // Установим тайм-аут для чтения тела сообщения
        deadline_.expires_after(std::chrono::seconds(READ_TIMEOUT));

        // Асинхронно читаем само сообщение
        // _1 будет заменён на объект boost::system::error_code,
        //     который предоставляет информацию об ошибке (или её отсутствии).
        // _2 будет заменён на размер данных (тип std::size_t),
        //     указывающий, сколько байт было успешно прочитано.
        boost::asio::async_read(
            socket_,
            boost::asio::buffer(read_msg_.data(), msg_length),
            strand_.wrap(boost::bind(&PersonInRoom::ReadHandler,
                                     shared_from_this(), _1, _2)));
    } else {
		LOG_MSG("PersonInRoom::HEaderHandler LEaving");
        room_.Leave(shared_from_this());
    }
}

void PersonInRoom::OnMessage(const std::vector<unsigned char>& msg) {
    LOG_ERR("Adding message to queue");
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    LOG_ERR("Added message to write queue, size: " << msg.size());
    if (!write_in_progress) {
        LOG_ERR("Starting async_write");
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(write_msgs_.front().data(),
                                write_msgs_.front().size()),
            strand_.wrap(boost::bind(&PersonInRoom::WriteHandler,
                                     shared_from_this(), _1)));
    }
}

void PersonInRoom::ReadHandler(
    const boost::system::error_code& error
    , size_t bytes_readed
    )
{
    if (!error) {
        std::vector<unsigned char> received_msg(read_msg_.begin(), read_msg_.end());

        // Debug print
        uint16_t received_msg_size = received_msg.size();
        LOG_HEX("Received message size in hex", received_msg_size, 2);
        LOG_VEC("Received message", received_msg);

        // Отмена таймера после успешного чтения
        deadline_.expires_at(boost::asio::steady_timer::time_point::max());

        room_.Broadcast(received_msg, shared_from_this());

        // готовим буфер для чтения следующего заголовка
        read_msg_.resize(2);
        // снова читаем заголовок
        boost::asio::async_read(
            socket_,
            boost::asio::buffer(read_msg_.data(), 2),
            strand_.wrap(boost::bind(&PersonInRoom::HeaderHandler,
                                     shared_from_this(), _1)));
    } else {
		LOG_MSG("PersonInRoom::ReadHandler LEaving");
        room_.Leave(shared_from_this());
    }
}

void PersonInRoom::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        LOG_ERR("Message written successfully");

        write_msgs_.pop_front();

        if (!write_msgs_.empty()) {
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_msgs_.front().data(),
                                    write_msgs_.front().size()),
                strand_.wrap(boost::bind(&PersonInRoom::WriteHandler,
                                         shared_from_this(), _1)));
        }
    } else {
        LOG_ERR("Message written successfully: " << error.message());
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
