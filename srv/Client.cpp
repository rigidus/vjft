// Client.cpp
#include "Client.hpp"

std::string client_private_key_file;
std::vector<std::string> recipient_public_key_files;


Client::Client(const std::array<char, MAX_NICKNAME>& nickname,
               boost::asio::io_service& io_service,
               tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service), socket_(io_service) {
    std::cout << ":> Client::Client(): Initializing async connect" << std::endl;
    std::cout << ":> Client::Client(): io_service initialized" << std::endl;
    std::cout << ":> Client::Client(): socket open: "
              << std::boolalpha << socket_.is_open() << std::endl;
    std::cout << ":> Client::Client(): endpoint_iterator valid: "
              << std::boolalpha
              << static_cast<bool>(endpoint_iterator != tcp::resolver::iterator())
              << std::endl;

    // if (endpoint_iterator != tcp::resolver::iterator()) {
    //     socket_.async_connect(*endpoint_iterator,
    //                           boost::bind(&Client::OnConnect, this, _1));
    // } else {
    //     std::cerr << "Client::Client(): Invalid endpoint_iterator" << std::endl;
    // }

    // strcpy(nickname_.data(), nickname.data());
    // memset(read_msg_.data(), '\0', MAX_IP_PACK_SIZE);

    std::string password;
    std::cout << ":> Enter password for private key: ";
    std::cin >> password;
    client_private_key_ = LoadKeyFromFile(client_private_key_file, true, password);
    if (!client_private_key_) {
        abort();
    }

    // Загружаем публичные ключи получателей
    if (!recipient_public_key_files.empty()) {
        // Проходим по файлам ключей и загружаем их
        for (const auto& key_file : recipient_public_key_files) {
            EVP_PKEY* public_key = LoadKeyFromFile(key_file, false);
            recipient_public_keys.push_back(public_key);
            std::string fingerprint = GetPubKeyFingerprint(public_key);
            recipient_public_keys_fingerprints.push_back(fingerprint);
        }
    }

    boost::asio::async_connect(socket_,
                               endpoint_iterator,
                               boost::bind(&Client::OnConnect, this, _1));

    std::cerr << ":> Client::Client(): Async Connect ok" << std::endl;
}

void Client::Write(const std::vector<char>& msg) {
    std::cout << ":> Client::Write() Scheduling message write" << std::endl;
    io_service_.post(boost::bind(&Client::WriteImpl, this, msg));
}

void Client::Close() {
    io_service_.post(boost::bind(&Client::CloseImpl, this));
}

void Client::OnConnect(const boost::system::error_code& error) {
    if (!error) {
        std::cout << ":> Client::OnConnect(): Connection successful, "
                  << "starting to read header" << std::endl;
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
        std::cout << ":> HeaderHandler: Message length = " << msg_length << std::endl;
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
    // std::string msg_data = read_msg_.data();
    // std::cout << ":> ReadHandler:\"" << msg_data << "\"" << std::endl;

    if (!error) {
        std::vector<char> received_msg(read_msg_.begin(), read_msg_.end());

        // dbgout
        std::string msg_data(received_msg.begin(), received_msg.end());
        std::cout << ":> Client::ReadHandler(): Received message: [" << msg_data << "]"
                  << std::endl;

        // Десериализуем Map из строки
        std::map<std::string, std::string> data = Message::unpack(msg_data);

        std::string decoded_content, decrypted_message, signature_str;
        std::vector<unsigned char> decoded_message, signature;

        decoded_message = Crypt::Base64Decode(data["enc"]);
        dbg_out_vec("Decoded message", decoded_message);

        if (!decoded_message.empty()) {
            auto decrypted_msg =
                Crypt::decipher(client_private_key_, recipient_public_keys[0],
                                decoded_message);

            if (decrypted_msg.empty()) {
                std::cerr << ":> Decryption error or message not for me " << std::endl;
            } else {
                std::cout << ":> Message received successfully: " << decrypted_msg
                          << std::endl;
                for (const auto& pair : data) {
                    std::cout << pair.first << ": " << pair.second << std::endl;
                }
            }
        }

        // Снова начинаем чтение заголовка следующего сообщения
        read_msg_.resize(2); // готовим буфер для чтения следующего заголовка
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), 2),
                                boost::bind(&Client::HeaderHandler, this, _1));
    } else {
        std::cerr << ":> Client::ReadHandler(): Error reading message: "
                  << error.message() << std::endl;
        CloseImpl();
    }
}

void Client::WriteImpl(std::vector<char> msg) {
    std::cout << ":> Client::WriteImpl()" << std::endl;

    bool write_in_progress = !write_msgs_.empty();

    std::string message(msg.data());

    std::map<std::string, std::string> data;

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
        // Шифрование и кодирование в Base64 поля content
        auto encrypted_msg =
            Crypt::encipher(client_private_key_, recipient_public_keys[i], message);

        dbg_out_vec("Encrypted message", encrypted_msg);

        std::string encoded = Crypt::Base64Encode(encrypted_msg);
        // Map
        data = {
            {"to", to},
            {"msg", message},
            {"enc", encoded},
            {"sign", signature}
        };
    }
    // Отладочный вывод Map
    for (const auto& pair : data) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    // Сериализуем Map в строку
    std::string packed_message = Message::pack(data);
    // Проверка длины упакованного сообщения (TODO: уточнить макс размер)
    if (packed_message.size() > MAX_IP_PACK_SIZE) {
        // TODO: тут нужно разбивать сообщение на блоки, шифровать их по отдельности,
        // нумеровать и отправлять в сокет, но пока мы просто не отправляем
        std::cerr << ":> Error: Message is too large to fit in the buffer" << std::endl;
        return;
    }

    std::cout << ":> Client::WriteImp(): Packed Message Size: " << packed_message.size()
              << std::endl;

    // Инициализация вектора данными из packed_message
    std::vector<char> formatted_msg(packed_message.begin(), packed_message.end());
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

    std::cout << ":> Client::WriteImpl(): "
              << "Formatted message with length prefix prepared" << std::endl;

    // Теперь добавляем formatted_msg в очередь сообщений на отправку
    write_msgs_.push_back(std::move(formatted_msg));

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

    std::cout << ":> Client::WriteImpl(): End func" << std::endl;
}

void Client::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        std::cout << ":> Client::WriteHandler(): Message written successfully"
                  << std::endl;
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
        std::cerr << ":> Client::WriteHandler: Error writing message: "
                  << error.message() << std::endl;
        CloseImpl();
    }
}

void Client::CloseImpl() {
    std::cout << ":> CloseImpl: Closing socket" << std::endl;
    socket_.close();
}


std::string Client::GetPubKeyFingerprint(EVP_PKEY* public_key) {
    unsigned char* der = nullptr;
    int len = i2d_PUBKEY(public_key, &der);
    if (len < 0) {
        std::cerr << ":> Client::GetPubKeyFingerprint(): PubKeyFingerprint error: "
                  << "Failed to convert PubKey to DER format"
                  << std::endl;
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (!EVP_Digest(der, len, hash, &hash_len, EVP_sha256(), nullptr)) {
        OPENSSL_free(der);
        std::cerr << ":> Client::GetPubKeyFingerprint(): PubKeyFingerprint error: "
                  << "Failed to compute SHA-256 hash of public key"
                  << std::endl;
        return "";
    }
    OPENSSL_free(der);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }

    return ss.str();
}

#define _CHUNK_SIZE 127

std::vector<std::string> splitString(
    const std::string& input, std::size_t chunk_size)
{
    std::vector<std::string> chunks;
    std::size_t length = input.size();

    for (std::size_t i = 0; i < length; i += chunk_size) {
        chunks.push_back(input.substr(i, chunk_size));
    }

    return chunks;
}


EVP_PKEY* Client::LoadKeyFromFile(const std::string& key_file, bool is_private,
                                  const std::string& password) {
    FILE* fp = fopen(key_file.c_str(), "r");
    if (!fp) {
        std::cerr << "Error opening key file: " << key_file << std::endl;
        return nullptr;
    }

    EVP_PKEY* key = nullptr;
    if (is_private) {
        key = PEM_read_PrivateKey(fp, nullptr, nullptr,
                                  const_cast<char*>(password.c_str()));
    } else {
        key = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    }
    fclose(fp);

    if (!key) {
        std::cerr << "Error loading key: "
                  << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
        return nullptr;
    }

    return key;
    // TODO: Надо освобождать (если удалось загрузить) ключи
    // с помощью EVP_PKEY_free(key);
}
