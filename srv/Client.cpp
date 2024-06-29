// Client.cpp
#include "Client.hpp"

std::string client_private_key_file;
std::vector<std::string> recipient_public_key_files;

Client::Client(const std::array<char, MAX_NICKNAME>& nickname,
               boost::asio::io_service& io_service,
               tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service),
      socket_(io_service),
      read_timeout_timer_(io_service)
{
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

void Client::FindSyncMarker() {
    // Буфер для одного байта
    auto buffer = std::make_shared<std::vector<unsigned char>>(1);
    // Начинаем чтение одного байта асинхронно
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(*buffer, 1),
        [this, buffer](const boost::system::error_code& error, std::size_t) {
            if (!error) {
                // Проверяем нулевой байт и обновляем счетчик
                if ((*buffer)[0] == 0x00) {
                    ++zero_byte_count_;
                } else {
                    zero_byte_count_ = 0;
                }

                // Проверяем, достигли ли мы 32 нулевых байт подряд
                if (zero_byte_count_ >= 32) {
                    zero_byte_count_ = 0;
                    LOG_MSG("Sync marker found, resuming normal operation");
                    // Считываем длину следующего сообщения
                    read_msg_.resize(2);
                    boost::asio::async_read(
                        socket_,
                        boost::asio::buffer(read_msg_.data(), 2),
                        boost::bind(&Client::HeaderHandler, this, _1));
                } else {
                    // Продолжаем поиск маркера
                    FindSyncMarker();
                }
            } else {
                LOG_ERR("Error during sync marker search: " + error.message());
                CloseImpl();
            }
        });
}

bool Client::checkSyncMarkerInQueue() {
    // Проверяем наличие 32 нулевых байт подряд в очереди
    int zero_count = 0;
    for (auto byte : read_queue_) {
        if (byte == 0x00) {
            zero_count++;
            if (zero_count >= SYNC_MARKER_SIZE) return true;
        } else {
            zero_count = 0;
        }
    }
    return false;
}

void Client::HandleDataTimeout(const boost::system::error_code& error) {
    if (error != boost::asio::error::operation_aborted) {
        // Таймер не был отменен, значит произошло реальное истечение времени
        LOG_ERR("Data read timeout occurred");

        // Здесь мы можем попытаться закрыть текущее соединение
        // и начать процедуру восстановления соединения
        CloseImpl();

        // Опционально, можно начать процедуру восстановления
        // соединения или повторного запроса
        Recover();  // Функция Recover должна содержать логику для обработки сбоя чтения
    } else {
        // Таймер был отменен, что означает успешное завершение операции чтения
        LOG_MSG("Data timeout timer was cancelled, data received successfully");
    }
}

void Client::Recover() {
    LOG_ERR("Recovering from an error");

    // Отмена всех асинхронных операций, если это возможно
    boost::system::error_code ec;
    socket_.cancel(ec);
    if (ec) {
        LOG_ERR("Error cancelling socket operations: " + ec.message());
    }

    // Закрытие сокета
    socket_.close(ec);
    if (ec) {
        LOG_ERR("Error closing socket: " + ec.message());
    }

    // Очистка всех буферов и очередей
    read_queue_.clear();
    std::vector<unsigned char>().swap(read_msg_); // Освобождение памяти

    // Переподключение или повторная инициализация, если необходимо
    // boost::asio::ip::tcp::resolver resolver(io_service_);
    // auto endpoint_iterator = resolver.resolve({server_ip, server_port});
    // socket_.open(endpoint_iterator->endpoint().protocol(), ec);
    // if (!ec) {
    //     socket_.async_connect(*endpoint_iterator,
    //                           boost::bind(&Client::OnConnect, this, _1));
    // } else {
    //     LOG_ERR("Error re-opening socket: " + ec.message());
    // }

    // Уведомление пользователя или системы, если необходимо
    // Можно реализовать отправку уведомления через другие механизмы
}


void Client::HeaderHandler(const boost::system::error_code& error) {
    if (!error) {
        // В данный момент мы знаем, что 2 байта длины пакета прочитаны
        // и находятся в поле класса read_msg_
        // Формируем два байта длины
        uint16_t msg_length =
            (static_cast<uint16_t>(read_msg_[1]) << 8) |
            static_cast<uint16_t>(read_msg_[0]);

        // Добавляем прочитанные байты длины в очередь чтения
        read_queue_.push_back(read_msg_[0]);
        read_queue_.push_back(read_msg_[1]);

        // Проверяем, не содержит ли очередь синхромаркер
        if (checkSyncMarkerInQueue()) {
            // Если очередь содержит синхромаркер, которого мы не ожидаем,
            // то переходим к Recover
            Recover();
            return;
        }

        // Проверка длины полученного сообщения
        if (msg_length > MIN_PACK_SIZE) {
            // Слишком маленькая длина
            LOG_ERR("Error: Message length is too SMALL: " << msg_length);
            LOG_HEX("msg_length in hex", msg_length, 2);
            Recover();
        } else if (msg_length > MAX_PACK_SIZE) {
            // Слишком большая длина
            LOG_ERR("Error: Message length is too LARGE: " << msg_length);
            LOG_HEX("msg_length in hex", msg_length, 2);
            Recover();
            return;
        }

        LOG_MSG("Message length (-2 bytes length): " << msg_length);

        // Если мы тут, то все в порядке
        // Обрезаем очередь до последних READ_QUEUE_SIZE элементов
        if (read_queue_.size() > READ_QUEUE_SIZE) {
            read_queue_.erase(
                read_queue_.begin(),
                read_queue_.begin() + (read_queue_.size() - READ_QUEUE_SIZE));
        }

        // Подготавливаем размер буфера для чтения данных сообщения
        read_msg_.resize(msg_length);

        // Запускаем таймер ожидания данных (мы получили длину, ждем остальное)
        read_timeout_timer_.expires_from_now(
            boost::posix_time::seconds(boost::posix_time::seconds(READ_TIMEOUT)));
        read_timeout_timer_.async_wait(
            boost::bind(&Client::HandleDataTimeout, this, _1));

        // Читаем остальные данные асинхронно
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), msg_length),
                                boost::bind(&Client::ReadHandler, this, _1, _2));
    } else {
        CloseImpl();
    }
}



void Client::ReadHandler(
    const boost::system::error_code& error,
    size_t bytes_readed)
{
    if (!error) {
        // Отменяем таймер, поскольку данные были успешно прочитаны
        read_timeout_timer_.cancel();

        // Тут мы уже получаем само сообщение, без его длины
        // потому что длина была отрезана в HeaderHandler
        std::vector<unsigned char> received_msg(read_msg_.begin(), read_msg_.end());

        uint16_t received_msg_size = received_msg.size();

        // Debug print
        LOG_MSG("Received message size: " << received_msg_size);
        LOG_HEX("Received message size in hex", received_msg_size, 2);
        LOG_VEC("Received message", received_msg);

        // TODO: перед декодированием надо отрезать синхромаркер
        if (received_msg.size() > 32) {
            received_msg.resize(received_msg.size() - 32);  // Обрезаем синхромаркер
        }

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

    // sync_marker : 32 нулевых байта
    std::vector<unsigned char> sync_marker(32, 0);

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

        // Вставка синхромаркера в конец packed_msg
        packed_msg.insert(packed_msg.end(), sync_marker.begin(), sync_marker.end());

        // Вычисляем размер packed_msg + sync_marker_size
        uint16_t pack_sync_size =
            static_cast<uint16_t>(packed_msg.size());

        // Помещаем длину вперед
        unsigned char len_bytes[2];
        len_bytes[0] = // младший байт первым
            static_cast<unsigned char>(pack_sync_size & 0xFF);
        len_bytes[1] = // старший байт вторым
            static_cast<unsigned char>((pack_sync_size >> 8) & 0xFF);
        // Вставка двух байтов длины в начало packed_msg перед содержимым
        packed_msg.insert(packed_msg.begin(), len_bytes, len_bytes + 2);

        // Вычисляем длину packed_msg_size = длина + содержимое + синхромаркер
        uint16_t packed_msg_size = static_cast<uint16_t>(packed_msg.size());

        // Debug print packed msg size
        LOG_HEX("Packed msg size in hex", packed_msg_size, 2);
        LOG_MSG("Packed msg size (+2): " << packed_msg.size());
        LOG_VEC("Packed msg" , packed_msg);

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
