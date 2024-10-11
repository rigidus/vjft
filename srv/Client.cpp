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
    LOG_ERR("Initializing async connect");
    LOG_ERR("Io_service initialized");
    LOG_ERR("socket open: "  << std::boolalpha << socket_.is_open());
    LOG_ERR("Endpoint_iterator valid: " << std::boolalpha
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
    client_private_key_ =
        Crypt::LoadKeyFromFile(client_private_key_file, true, password);
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

    LOG_MSG("Keys loaded");

    boost::asio::async_connect(socket_,
                               endpoint_iterator,
                               boost::bind(&Client::OnConnect, this, _1));

    LOG_ERR("Async Connect ok");
}

void Client::Write(const std::vector<unsigned char>& msg) {
    LOG_ERR("Scheduling message write");
    io_service_.post(boost::bind(&Client::WriteImpl, this, msg));
}

void Client::Close() {
    io_service_.post(boost::bind(&Client::CloseImpl, this));
}

void Client::OnConnect(const boost::system::error_code& error) {
    if (!error) {
        LOG_ERR("Connection successful, starting to read header");
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
                    LOG_ERR("Sync marker found, resuming normal operation");
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
        LOG_ERR("Data timeout timer was cancelled, data received successfully");
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
        uint16_t need_read_size =
            (static_cast<uint16_t>(read_msg_[1]) << 8) |
            static_cast<uint16_t>(read_msg_[0]);

        LOG_HEX("need read (pack_sync_size-2) (hex)", need_read_size, 2);

        // Добавляем прочитанные байты длины в очередь чтения
        read_queue_.push_back(read_msg_[0]);
        read_queue_.push_back(read_msg_[1]);

        // Проверяем, не содержит ли очередь синхромаркер
        if (checkSyncMarkerInQueue()) {
            // Если очередь содержит синхромаркер, которого мы не ожидаем,
            // то переходим к Recover
            LOG_ERR("Error: Sync marker spotted");
            Recover();
            return;
        }

        // Проверка длины полученного сообщения
        if (need_read_size < MIN_PACK_SIZE) {
            // Слишком маленькая длина
            LOG_HEX("Error: Message length is too SMALL (hex)", need_read_size, 2);
            LOG_HEX("Expected (hex)", MIN_PACK_SIZE, 2);
            Recover();
        } else if (need_read_size > MAX_PACK_SIZE) {
            // Слишком большая длина
            LOG_HEX("Error: Message length is too LARGE (hex) ", need_read_size, 2);
            LOG_HEX("Expected (hex)", MAX_PACK_SIZE, 2);
            Recover();
            return;
        }

        // Если мы тут, то все в порядке
        // Обрезаем очередь до последних READ_QUEUE_SIZE элементов
        if (read_queue_.size() > READ_QUEUE_SIZE) {
            read_queue_.erase(
                read_queue_.begin(),
                read_queue_.begin() + (read_queue_.size() - READ_QUEUE_SIZE));
        }

        // Подготавливаем размер буфера для чтения данных сообщения
        read_msg_.resize(need_read_size);

        // Запускаем таймер ожидания данных (мы получили длину, ждем остальное)
        read_timeout_timer_.expires_from_now(
            boost::posix_time::seconds(boost::posix_time::seconds(READ_TIMEOUT)));
        read_timeout_timer_.async_wait(
            boost::bind(&Client::HandleDataTimeout, this, _1));

        // Читаем остальные данные асинхронно
        // _1 будет заменён на объект boost::system::error_code,
        //     который предоставляет информацию об ошибке (или её отсутствии).
        // _2 будет заменён на размер данных (тип std::size_t),
        //     указывающий, сколько байт было успешно прочитано.
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_.data(), need_read_size),
                                boost::bind(&Client::ReadHandler, this, _1, _2));
    } else {
        CloseImpl();
    }
}


void Client::ReadHandler(
    const boost::system::error_code& error,
    size_t bytes_readed)
{
    if (error) {
        // Если случилась ошибка - закрываем сокет и выходим
        LOG_ERR("Error reading message: " << error.message());
        CloseImpl();
        return;
    }

    // Проверяем, соответствует ли количество прочитанных байт ожидаемому
    if (bytes_readed != read_msg_.size()) {
        LOG_ERR("Error: Mismatch in the number of bytes read. Expected: "
                + std::to_string(read_msg_.size()) + ", Read: "
                + std::to_string(bytes_readed));
        Recover();
        return;
    }

    // Отменяем таймер, поскольку данные были успешно прочитаны
    read_timeout_timer_.cancel();

    // Тут мы уже получаем само сообщение, без его длины
    // потому что длина была отрезана в HeaderHandler,
    // но sync_marker в конце присутствует
    std::vector<unsigned char> received_msg(read_msg_.begin(), read_msg_.end());

    uint16_t received_msg_size = received_msg.size();

    // Debug print
    LOG_HEX("Received message size in hex", received_msg_size, 2);
    LOG_VEC("Received message", received_msg);


    if (read_msg_.size() < SYNC_MARKER_SIZE) {
        // По идее, сообщение не может быть размером меньше чем sync_marker
        // но если в будущем это изменится, то проверка пригодится
        LOG_ERR("Sync marker not found or not in the correct position.");
        Recover();
    }

    // Проверяем наличие синхромаркера в конце сообщения
    bool sync_marker_found = true;
    for (size_t i = read_msg_.size() - SYNC_MARKER_SIZE; i < read_msg_.size(); i++)
    {
        if (read_msg_[i] != 0) {
            sync_marker_found = false;
            break;
        }
    }
    if (!sync_marker_found) {
        LOG_ERR("Sync marker not found or not in the correct position.");
        Recover();
        return;
    }

    // Перед декодированием надо отрезать sync_marker
    received_msg.resize(received_msg.size() - SYNC_MARKER_SIZE);


    // Для каждого из известных нам абонентов
    for (auto i = 0; i < recipient_public_keys.size(); ++i) {
        std::string decrypted_msg =
            Crypt::decipher(client_private_key_, recipient_public_keys[i],
                            received_msg);
        if (decrypted_msg.empty()) {
            LOG_ERR("Received message is not for me");
        } else {
            LOG_MSG(decrypted_msg);
        }
    }
    // Снова начинаем чтение заголовка следующего сообщения
    read_msg_.resize(2); // готовим буфер для чтения следующего заголовка
    boost::asio::async_read(socket_,
                            boost::asio::buffer(read_msg_.data(), 2),
                            boost::bind(&Client::HeaderHandler, this, _1));
}

void Client::WriteImpl(std::vector<unsigned char> msg) {
    LOG_ERR("");

    bool write_in_progress = !write_msgs_.empty();

    // Строка нужна для передачи криптору
    std::string msg_str(msg.begin(), msg.end());

    // sync_marker : 32 нулевых байта
    std::vector<unsigned char> sync_marker(32, 0);

    // Для каждого из ключей получателей..
    for (auto i = 0; i < recipient_public_keys.size(); ++i) {
        // Шифрование
        std::vector<unsigned char> encrypted_msg =
            Crypt::encipher(client_private_key_, recipient_public_keys[i], msg_str);

        LOG_VEC("Encrypted message", encrypted_msg);

        // Debug print encrypted msg size
        uint16_t encrypted_msg_size = static_cast<uint16_t>(encrypted_msg.size());
        LOG_HEX("encrypted_msg_size [envelope_chunk_size[envelope]] (hex)",
                encrypted_msg_size, 2);

        // Инициализация вектора packed_msg данными из encrypted_msg
        std::vector<unsigned char> packed_msg(
            encrypted_msg.begin(), encrypted_msg.end());

        // Вставка синхромаркера в конец packed_msg
        // [envelope_chunk_size[envelope]]+[sync_marker]
        packed_msg.insert(packed_msg.end(), sync_marker.begin(), sync_marker.end());

        // Вычисляем размер [envelope_chunk_size[envelope]]+[sync_marker]
        uint16_t pack_sync_size =
            static_cast<uint16_t>(packed_msg.size());

        // Помещаем длину вперед
        unsigned char len_bytes[2];
        len_bytes[0] = // младший байт первым
            static_cast<unsigned char>(pack_sync_size & 0xFF);
        len_bytes[1] = // старший байт вторым
            static_cast<unsigned char>((pack_sync_size >> 8) & 0xFF);
        // Вставка двух байтов длины в начало packed_msg
        // [pack_sync_size[[envelope_chunk_size[envelope]]+[sync_marker]]]
        packed_msg.insert(packed_msg.begin(), len_bytes, len_bytes + 2);

        // Вычисляем длину packed_msg_size
        // [pack_sync_size[[envelope_chunk_size[envelope]]+[sync_marker]]]
        uint16_t packed_msg_size = static_cast<uint16_t>(packed_msg.size());

        // Debug print packed msg size
        LOG_HEX("packed_msg_size = [pack_sync_size[[envelope_chunk_size[envelope]]+[sync_marker]]] (hex)", packed_msg_size, 2);
        LOG_VEC("packed_msg" , packed_msg);

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
}

void Client::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        LOG_ERR("Message written successfully");

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
