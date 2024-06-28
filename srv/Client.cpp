// Client.cpp
#include "Client.hpp"

std::string client_private_key_file;
std::vector<std::string> recipient_public_key_files;

Client::Client(const std::array<char, MAX_NICKNAME>& nickname,
               boost::asio::io_service& io_service,
               tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service), socket_(io_service) {
    LOG_MSG("Initializing async connect");
    LOG_MSG("Io_service initialized");
    LOG_MSG("socket open: "  << std::boolalpha << socket_.is_open());
    LOG_MSG("Endpoint_iterator valid: " << std::boolalpha
            << static_cast<bool>(endpoint_iterator != tcp::resolver::iterator()));

    // if (endpoint_iterator != tcp::resolver::iterator()) {
    //     socket_.async_connect(*endpoint_iterator,
    //                           boost::bind(&Client::OnConnect, this, _1));
    // } else {
    //     std::cerr << "Client::Client(): Invalid endpoint_iterator" << std::endl;
    // }

    // strcpy(nickname_.data(), nickname.data());
    // memset(read_msg_.data(), '\0', MAX_IP_PACK_SIZE);

    std::string password;
    std::cout << "=> Enter password for private key: ";
    std::cin >> password;
    client_private_key_ = Crypt::LoadKeyFromFile(client_private_key_file, true, password);
    if (!client_private_key_) {
        abort();
    }

    // Загружаем публичные ключи получателей
    if (!recipient_public_key_files.empty()) {
        // Проходим по файлам ключей и загружаем их
        for (const auto& key_file : recipient_public_key_files) {
            EVP_PKEY* public_key = Crypt::LoadKeyFromFile(key_file, false);
            recipient_public_keys.push_back(public_key);
            std::string fingerprint = Crypt::GetPubKeyFingerprint(public_key);
            recipient_public_keys_fingerprints.push_back(fingerprint);
        }
    }

    boost::asio::async_connect(socket_,
                               endpoint_iterator,
                               boost::bind(&Client::OnConnect, this, _1));

    std::cerr << ":> Client::Client(): Async Connect ok" << std::endl;
}

void Client::Write(const std::vector<unsigned char>& msg) {
    LOG_MSG("Scheduling message write");
    io_service_.post(boost::bind(&Client::WriteImpl, this, msg));
}

void Client::Close() {
    io_service_.post(boost::bind(&Client::CloseImpl, this));
}

void Client::OnConnect(const boost::system::error_code& error) {
    if (!error) {
        LOG_MSG("Connection successful, starting to read header");
        // Временно не передаем никнейм
        // boost::asio::async_write(socket_,
        //                          boost::asio::buffer(nickname_, nickname_.size()),
        //                          boost::bind(&Client::ReadHandler, this, _1));
        // Сразу начинаем чтение сообщений после установления соединения
        // читаем сначала 2 байта заголовка
        read_msg_.resize(2);
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), 2),
                                boost::bind(&Client::HeaderHandler, this, _1));
    } else {
        std::cerr << "\nOnConnect: Connection failed: " << error.message() << std::endl;
        CloseImpl();
    }
}

void Client::HeaderHandler(const boost::system::error_code& error) {
    if (!error) {
        uint16_t msg_length =
            (static_cast<uint16_t>(read_msg_[1]) << 8) |
            static_cast<uint16_t>(read_msg_[0]);

        // Проверка длины полученного сообщения (TODO: уточнить макс размер)
        if (msg_length > MAX_IP_PACK_SIZE) {
            // TODO: тут нужно разбивать сообщение на блоки,
            // шифровать их по отдельности,
            // нумеровать и отправлять в сокет, но пока мы просто не отправляем
            LOG_ERR(":> Error: Message is too large to fit in the buffer");
            CloseImpl();
            return;
        }

        LOG_MSG("Message length: " << msg_length);
        read_msg_.resize(msg_length);
        // Читаем остальную часть сообщения
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), msg_length),
                                boost::bind(&Client::ReadHandler, this, _1));
    } else {
        CloseImpl();
    }
}

void Client::ReadHandler(const boost::system::error_code& error) {
    if (!error) {
        // Тут мы уже получаем само сообщение, без его длины
        // потому что длина была отрезана в HeaderHandler
        std::vector<unsigned char> received_msg(read_msg_.begin(), read_msg_.end());

        std::string msg_data(received_msg.begin(), received_msg.end());

        // dbgout
        // LOG_MSG("Received message: [" << msg_data << "]");
        // // Десериализуем Map из строки
        // std::map<std::string, std::string> data = Message::unpack(msg_data);
        // // for (const auto& pair : data) {
        // //     std::cout << ">> " << pair.first << ": "
        // //               << pair.second << std::endl;
        // // }

        std::vector<unsigned char> decoded_message;

        // decoded_message = Crypt::Base64Decode(data["enc"]);
        decoded_message = Crypt::Base64Decode(msg_data);
        // dbg_out_vec("Decoded message", decoded_message);

        if (!decoded_message.empty()) {
            std::string decrypted_msg =
                Crypt::decipher(client_private_key_, recipient_public_keys[0],
                                decoded_message);

            if (decrypted_msg.empty()) {
                LOG_MSG("Received message is not for me");
            } else {
                LOG_MSG("Message received successfully: " << decrypted_msg);
            }
        }

        // Снова начинаем чтение заголовка следующего сообщения
        read_msg_.resize(2); // готовим буфер для чтения следующего заголовка
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), 2),
                                boost::bind(&Client::HeaderHandler, this, _1));
    } else {
        LOG_ERR("Error reading message: " << error.message());
        CloseImpl();
    }
}

void Client::WriteImpl(std::vector<unsigned char> msg) {
    LOG_MSG("");

    bool write_in_progress = !write_msgs_.empty();

    std::string message(reinterpret_cast<char*>(msg.data()));

    // Подпишем сообщение своим приватным ключом
    std::string signature = "";
    auto optSign = Crypt::SignMsg(message, client_private_key_);
    if (!optSign) { return; } else {
        signature = Crypt::Base64Encode(*optSign);
    }
    // Для каждого из ключей получателей..
    for (auto i = 0; i < recipient_public_keys.size(); ++i) {
        // Определить 'to' как отпечаток ключа
        std::string to = recipient_public_keys_fingerprints[i];

        // Шифрование
        std::vector<unsigned char> encrypted_msg =
            Crypt::encipher(client_private_key_, recipient_public_keys[i], message);

        dbg_out_vec("Encrypted message", encrypted_msg);

        // И кодирование в Base64
        std::string encoded = Crypt::Base64Encode(encrypted_msg);

        LOG_MSG("Encoded: " << encoded);

        // // Map
        // std::map<std::string, std::string> data;
        // data = {
        //     // {"to", to},
        //     // {"msg", message},
        //     {"enc", encoded},
        //     // {"sign", signature}
        // };

        // // Отладочный вывод Map
        // for (const auto& pair : data) {
        //     std::cout << ">> " << pair.first << ": " << pair.second << std::endl;
        // }

        // // Сериализуем Map в строку
        // std::string packed_message = Message::pack(data);

        std::string packed_message = encoded;

        // Проверка длины упакованного сообщения (TODO: уточнить макс размер)
        if (packed_message.size() > MAX_IP_PACK_SIZE) {
            // TODO: тут нужно разбивать сообщение на блоки,
            // шифровать их по отдельности,
            // нумеровать и отправлять в сокет, но пока мы просто не отправляем
            LOG_ERR(":> Error: Message is too large to fit in the buffer");
            return;
        } else {
            LOG_MSG("Packed Message Size: " << packed_message.size());
        }

        // Инициализация вектора данными из packed_message
        std::vector<unsigned char> formatted_msg(
            packed_message.begin(), packed_message.end());
        std::cout << ":> Client::WriteImpl(): Sending message of size: "
                  << formatted_msg.size()
                  << std::endl;
        // Помещаем длину вперед formatted_msg
        uint16_t formatted_msg_len = static_cast<uint16_t>(formatted_msg.size());
        char len_bytes[2];
        len_bytes[0] = static_cast<char>(formatted_msg_len & 0xFF); // младший байт
        len_bytes[1] = static_cast<char>((formatted_msg_len >> 8) & 0xFF); // старший
        // Вставка двух байтов длины в начало вектора
        formatted_msg.insert(formatted_msg.begin(), len_bytes, len_bytes + 2);

        LOG_MSG("Formatted message with length prefix prepared");

        // Теперь добавляем formatted_msg в очередь сообщений на отправку
        write_msgs_.push_back(std::move(formatted_msg));
    }

    // Если в данный момент запись не идет, инициируется асинхронная запись
    // сообщения в сокет. Когда асинхронная запись завершится, вызывается
    // функция-обработчик WriteHandler. Она проверит есть ли еще что-то
    // в очереди, и если есть - отправит сообщения асинхронно.
    if (!write_in_progress) {
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().size()),
            boost::bind(&Client::WriteHandler, this, _1));
    }

    LOG_MSG("End func");
}

void Client::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        LOG_MSG("Message written successfully");

        // Удаляем только что отправленное сообщение из очереди
        write_msgs_.pop_front();
        // Если очередь не пуста, инициируем новую асинхронную
        // запись следующего сообщения в сокет.
        if (!write_msgs_.empty()) {
            boost::asio::async_write(
                socket_,
                boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                boost::bind(&Client::WriteHandler, this, _1));
        }
    } else {
        LOG_ERR("Error writing message: " << error.message());
        CloseImpl();
    }
}

void Client::CloseImpl() {
    LOG_MSG("Closing socket");
    socket_.close();
}
