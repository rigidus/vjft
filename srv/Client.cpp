// Client.cpp
#include "Client.hpp"

std::string client_private_key_file;
std::vector<std::string> recipient_public_key_files;


Client::Client(const std::array<char, MAX_NICKNAME>& nickname,
               boost::asio::io_service& io_service,
               tcp::resolver::iterator endpoint_iterator)
    : io_service_(io_service), socket_(io_service) {
    strcpy(nickname_.data(), nickname.data());
    memset(read_msg_.data(), '\0', MAX_IP_PACK_SIZE);

    std::string password;
    std::cout << "Enter password for private key: ";
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

    boost::asio::async_connect(socket_, endpoint_iterator, boost::bind(&Client::OnConnect, this, _1));
}

void Client::Write(const std::array<char, MAX_IP_PACK_SIZE>& msg) {
    std::cout << "\nClient::Write()";
    io_service_.post(boost::bind(&Client::WriteImpl, this, msg));
}

void Client::Close() {
    io_service_.post(boost::bind(&Client::CloseImpl, this));
}

void Client::OnConnect(const boost::system::error_code& error) {
    if (!error) {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(nickname_, nickname_.size()),
                                 boost::bind(&Client::ReadHandler, this, _1));
    }
}

void Client::ReadHandler(const boost::system::error_code& error) {
    std::string msg_data = read_msg_.data();
    std::cout << "\nReadHandler:\"" << msg_data << "\"" << std::endl;
    if (!error) {
        std::map<std::string, std::string> data = Message::unpack(msg_data);

        bool encrypted = false;
        if (encrypted) {
//             std::string decoded_content, decrypted_message, signature_str;
//             std::vector<unsigned char> decoded_message, signature;

//             try {
//                 decoded_message = Base64Decode(data["content"]);
//                 decrypted_message = Client::DecryptMessage(decoded_message, client_private_key_);
//             } catch (const std::exception& e) {
//                 std::cerr << "Decryption error: " << e.what() << std::endl;
//                 CloseImpl();
//                 return;
//             }

//             if (CalculateChecksum(decrypted_message) != data["checksum"]) {
//                 std::cerr << "Checksum verification failed." << std::endl;
//                 CloseImpl();
//                 return;
//             }

//             signature_str = data["signature"];
//             signature = std::vector<unsigned char>(signature_str.begin(), signature_str.end());
//             EVP_PKEY* sender_public_key = LoadKeyFromFile(recipient_public_key_files[0], false);

//             try {
//                 if (!VerifySignature(decrypted_message, signature, sender_public_key)) {
//                     std::cerr << "Signature verification failed." << std::endl;
//                     EVP_PKEY_free(sender_public_key);
//                     CloseImpl();
//                     return;
//                 }
//             } catch (const std::exception& e) {
//                 std::cerr << "Verification error: " << e.what() << std::endl;
//                 EVP_PKEY_free(sender_public_key);
//                 CloseImpl();
//                 return;
//             }
//             EVP_PKEY_free(sender_public_key);

//             std::cout << "Message received successfully: " << decrypted_message << std::endl;
        } else {
            std::map<std::string, std::string> data = Message::unpack(msg_data);
            for (const auto& pair : data) {
                std::cout << pair.first << ": " << pair.second << std::endl;
            }
        }
        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_, read_msg_.size()),
                                boost::bind(&Client::ReadHandler, this, _1));
    } else {
        CloseImpl();
    }
}

void Client::WriteImpl(std::array<char, MAX_IP_PACK_SIZE> msg) {
    std::cout << "\nClient::WriteImpl" << std::endl;

    bool write_in_progress = !write_msgs_.empty();

    std::string message(msg.data());
    std::string length = std::to_string(message.size());
    std::string checksum = CalculateChecksum(message);
    std::map<std::string, std::string> data;
    // Определяем (todo), будем ли шифровать и подписывать сообщение
    // (некоторые управляющие сообщения не будут зашифрованы и подписаны)
    // (пока просто определяем как флаг)
    bool encrypted = true;

    if (encrypted) {
        // Подпишем сообщение своим приватным ключом
        std::string signature = "";
        auto optSign = SignMsg(message, client_private_key_);
        if (!optSign) { return; } else {
            signature = Base64Encode(*optSign);
        }
        // Для каждого из ключей получателей..
        for (auto i = 0; i < recipient_public_keys.size(); ++i) {
            // Определить 'to' как отпечаток ключа
            std::string to = recipient_public_keys_fingerprints[i];
            // Шифрование и кодирование в Base64 поля content
            std::optional<std::vector<unsigned char>> encrypted_msg =
                Client::EncryptMessage(message, recipient_public_keys[i]);
            std::string encoded = "";
            if (encrypted_msg) {
                encoded = Base64Encode(*encrypted_msg);
            }

            data = {
                {"to", to},
                {"len", length},
                {"crc", checksum},
                {"encf", encrypted ? "true" : "false"},
                {"msg", message},
                {"enc", encoded},
                {"sign", signature}
            };
        }
    } else {
        // Используем сообщение напрямую без шифрования
        data = {
            {"len", length},
            {"crc", checksum},
            {"encf", encrypted ? "true" : "false"},
            {"msg", message},
        };
    }

    for (const auto& pair : data) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    std::string packed_message = Message::pack(data);

    // Проверка длины упакованного сообщения
    if (packed_message.size() > MAX_IP_PACK_SIZE) {
        // TODO: тут нужно разбивать сообщение на блоки, шифровать их по отдельности,
        // нумеровать и отправлять в сокет, но пока мы просто не отправляем
        std::cerr << "Error: Message is too large to fit in the buffer" << std::endl;
        return;
    }

    std::array<char, MAX_IP_PACK_SIZE> formatted_msg;
    std::copy(packed_message.begin(), packed_message.end(), formatted_msg.begin());
    write_msgs_.push_back(formatted_msg);

    // Если в данный момент запись не идет, инициируется асинхронная запись
    // сообщения в сокет. Когда асинхронная запись завершится, вызывается
    // функция-обработчик WriteHandler. Она проверит есть ли еще что-то
    // в очереди, и если есть - отправит сообщения асинхронно.
    if (!write_in_progress) {
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
            boost::bind(&Client::WriteHandler, this, _1));
    }

    std::cout << "Client::WriteImplEnd" << std::endl;
}

void Client::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
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
        CloseImpl();
    }
}

void Client::CloseImpl() {
    socket_.close();
}


std::string Client::GetPubKeyFingerprint(EVP_PKEY* public_key) {
    unsigned char* der = nullptr;
    int len = i2d_PUBKEY(public_key, &der);
    if (len < 0) {
        std::cerr << "PubKeyFingerprint error: "
                  << "Failed to convert PubKey to DER format"
                  << std::endl;
        return "";
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (!EVP_Digest(der, len, hash, &hash_len, EVP_sha256(), nullptr)) {
        OPENSSL_free(der);
        std::cerr << "PubKeyFingerprint error: "
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


std::optional<std::vector<unsigned char>> Client::EncryptMessage(
    const std::string& message, EVP_PKEY* public_key)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, nullptr);
    if (!ctx) {
        std::cerr << "EncryptMessage error: "
                  << "Error creating context for encryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "EncryptMessage error: "
                  << "Error initializing encryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "EncryptMessage error: "
                  << "Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen,
                         reinterpret_cast<const unsigned char*>(message.c_str()),
                         message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "EncryptMessage error: "
                  << "Error determining buffer length for encryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_encrypt(
            ctx, out.data(), &outlen,
            reinterpret_cast<const unsigned char*>(message.c_str()), message.size())
        <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "EncryptMessage error: "
                  << "Error encrypting message" << std::endl;
        return std::nullopt;

    }

    EVP_PKEY_CTX_free(ctx);
    return out;
}


std::string Client::DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
        std::cout << "Error creating context for decryption";
        return "";
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cout << "Error initializing decryption";
        return "";
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cout << "Error setting RSA padding";
        return "";
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cout << "Error determining buffer length for decryption";
        return "";
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cout << "Error decrypting message";
        return "";
    }

    EVP_PKEY_CTX_free(ctx);
    return std::string(out.begin(), out.begin() + outlen);
}


std::string Client::CalculateChecksum(const std::string& message) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to initialize digest");
    }

    if (EVP_DigestUpdate(mdctx, message.c_str(), message.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to update digest");
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (EVP_DigestFinal_ex(mdctx, hash, &lengthOfHash) != 1) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Failed to finalize digest");
    }

    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < lengthOfHash; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool Client::VerifyChecksum(const std::string& message, const std::string& checksum) {
    return CalculateChecksum(message) == checksum;
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


std::optional<std::vector<unsigned char>> Client::SignMsg(const std::string& message,
                                                          EVP_PKEY* private_key) {
    // Создаем контекст для подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "Signing error: Failed to create EVP_MD_CTX" << std::endl;
        return std::nullopt;
    }

    // Инициализируем контекст для подписи с использованием алгоритма SHA-256
    if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error initializing signing: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error updating signing: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    // Определяем размер подписи
    size_t siglen;
    if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error finalizing signing: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    // Получаем подпись
    std::vector<unsigned char> signature(siglen);
    if (EVP_DigestSignFinal(mdctx, signature.data(), &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error getting signature: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    // Освобождаем контекст
    EVP_MD_CTX_free(mdctx);

    // Возвращаем подпись
    return signature;
}


bool Client::VerifySignature(const std::string& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) {
    // Создаем контекст для проверки подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // Инициализируем контекст для проверки подписи с использованием алгоритма SHA-256
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error initializing verification: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error updating verification: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    // Проверяем подпись
    int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    EVP_MD_CTX_free(mdctx);

    if (ret < 0) {
        throw std::runtime_error("Error finalizing verification: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    // Возвращаем true, если подпись верна, иначе false
    return ret == 1;
}

// Функция для кодирования Base64
std::string Client::Base64Encode(const std::vector<unsigned char>& buffer) {
    BIO* bio = BIO_new(BIO_f_base64());
    BIO* bmem = BIO_new(BIO_s_mem());
    bio = BIO_push(bio, bmem);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer.data(), buffer.size());
    BIO_flush(bio);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(bio, &bptr);

    std::string encoded(bptr->data, bptr->length);
    BIO_free_all(bio);

    return encoded;
}

// Функция для декодирования Base64
std::vector<unsigned char> Client::Base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), encoded.size());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    std::vector<unsigned char> buffer(encoded.size());
    int decoded_length = BIO_read(bio, buffer.data(), buffer.size());
    buffer.resize(decoded_length);
    BIO_free_all(bio);

    return buffer;
}
