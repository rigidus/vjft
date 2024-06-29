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
    // memset(read_msg_.data(), '\0', MAX_PACK_SIZE);

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

void Client::DbgHandler(const boost::system::error_code& error) {
    uint16_t msg_length =
        (static_cast<uint16_t>(read_msg_[1]) << 8) |
        static_cast<uint16_t>(read_msg_[0]);
    LOG_ERR("DBG: " << msg_length);
    // Для отладки зациклим чтение по 2 байта
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.data(), 2),
        boost::bind(&Client::DbgHandler, this, _1));
    return;
}

void Client::HeaderHandler(const boost::system::error_code& error) {
    if (!error) {
        uint16_t msg_length =
            (static_cast<uint16_t>(read_msg_[1]) << 8) |
            static_cast<uint16_t>(read_msg_[0]);

        // Проверка длины полученного сообщения (TODO: уточнить макс размер)
        if (msg_length > MAX_PACK_SIZE) {
            // TODO: тут нужно разбивать сообщение на блоки,
            // шифровать их по отдельности,
            // нумеровать и отправлять в сокет, но пока мы просто не отправляем
            LOG_ERR("Error: Message is too large to fit in the buffer: "
                    << msg_length);
            // CloseImpl();
            // return;

            // Для отладки зациклим чтение по 2 байта
            boost::asio::async_read(
                socket_,
                boost::asio::buffer(read_msg_.data(), 2),
                boost::bind(&Client::DbgHandler, this, _1));
        }

        LOG_MSG("Message length (-2 bytes length): " << msg_length);

        read_msg_.resize(msg_length);

        // Читаем остальную часть сообщения
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), msg_length),
                                boost::bind(&Client::ReadHandler, this, _1));
    } else {
        CloseImpl();
    }
}



void Client::ReadHandler(const boost::system::error_code& error)
{
    if (!error) {
        // Тут мы уже получаем само сообщение, без его длины
        // потому что длина была отрезана в HeaderHandler
        std::vector<unsigned char> received_msg(read_msg_.begin(), read_msg_.end());

        uint16_t received_msg_size = received_msg.size();

        // Debug print
        LOG_MSG("Received message size: " << received_msg_size);
        LOG_HEX("Received message size in hex", received_msg_size, 2);
        LOG_VEC("Received message", received_msg);

        std::string msg_data(received_msg.begin(), received_msg.end());

        LOG_MSG("Received message msg_data: [" << msg_data << "]");

        std::vector<unsigned char> decoded_message =
            Crypt::Base64Decode(msg_data);

        // Debug print decoded message
        LOG_VEC("Decoded message", decoded_message);

        if (decoded_message.empty()) {
            throw std::runtime_error("Base64 decode failed");
        }

        std::string decrypted_msg =
            Crypt::decipher(client_private_key_, recipient_public_keys[0],
                            decoded_message);

        if (decrypted_msg.empty()) {
            LOG_MSG("Received message is not for me");
        } else {
            LOG_MSG("Message received successfully: " << decrypted_msg);
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

    // Для каждого из ключей получателей..
    for (auto i = 0; i < recipient_public_keys.size(); ++i) {
        // Шифрование
        std::vector<unsigned char> encrypted_msg =
            Crypt::encipher(client_private_key_, recipient_public_keys[i], message);

        LOG_VEC("Encrypted message", encrypted_msg);

        // Debug print encrypted msg size
        uint16_t encrypted_msg_size = static_cast<uint16_t>(encrypted_msg.size());
        LOG_HEX("Encrypted message size in hex", encrypted_msg_size, 2);

        // Кодирование в Base64
        std::string base64encoded_msg = Crypt::Base64Encode(encrypted_msg);

        LOG_MSG("Base64 encoded message: " << base64encoded_msg);

        // Debug print base64 encoded msg size
        uint16_t base64encoded_msg_size =
            static_cast<uint16_t>(base64encoded_msg.size());
        LOG_HEX("Base64 encoded message size in hex", base64encoded_msg_size, 2);

        // Проверка длины упакованного сообщения (TODO: уточнить макс размер)
        if (base64encoded_msg.size() > MAX_PACK_SIZE) {
            // TODO: тут нужно разбивать сообщение на блоки,
            // шифровать их по отдельности,
            // нумеровать и отправлять, но пока мы просто не отправляем
            LOG_ERR("Error: Message is too large to fit in the buffer");
            return;
        }

        // Инициализация вектора packed_msg данными из base64encoded_msg
        std::vector<unsigned char> packed_msg(
            base64encoded_msg.begin(), base64encoded_msg.end());

        // Помещаем длину вперед packed_msg
        unsigned char len_bytes[2];
        len_bytes[0] = // младший байт первым
            static_cast<unsigned char>(base64encoded_msg_size & 0xFF);
        len_bytes[1] = // старший байт вторым
            static_cast<unsigned char>((base64encoded_msg_size >> 8) & 0xFF);
        // Вставка двух байтов длины в начало вектора
        packed_msg.insert(packed_msg.begin(), len_bytes, len_bytes + 2);

        // Вычисляем длину packed_msg_size
        uint16_t packed_msg_size = static_cast<uint16_t>(packed_msg.size());

        // Debug print packed msg size
        LOG_HEX("Packed msg size in hex", packed_msg_size, 2);
        LOG_MSG("Packed msg size (+2): " << packed_msg.size());

        // Теперь добавляем packed_msg в очередь сообщений на отправку
        write_msgs_.push_back(std::move(packed_msg));
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
