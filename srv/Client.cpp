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

    boost::asio::async_connect(socket_, endpoint_iterator, boost::bind(&Client::OnConnect, this, _1));
}

void Client::Write(const std::array<char, MAX_IP_PACK_SIZE>& msg) {
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
        bool encrypted;
        if (encrypted) {
            std::string decrypted_message;
            try {
                decrypted_message = Client::DecryptMessage(std::vector<unsigned char>(read_msg_.begin(), read_msg_.end()), client_private_key_);
            } catch (const std::exception& e) {
                std::cerr << "Decryption error: " << e.what() << std::endl;
                CloseImpl();
                return;
            }
            std::string signature_str = data["signature"];
            std::vector<unsigned char> signature(signature_str.begin(), signature_str.end());
            EVP_PKEY* sender_public_key = LoadKeyFromFile(recipient_public_key_files[0], false);
            try {
                if (VerifySignature(decrypted_message, signature, sender_public_key)) {
                    std::cout << "Signature verified successfully." << std::endl;
                } else {
                    std::cerr << "Signature verification failed." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Verification error: " << e.what() << std::endl;
            }
            EVP_PKEY_free(sender_public_key);
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
    bool write_in_progress = !write_msgs_.empty();

    std::string message(msg.data());
    std::string length = std::to_string(message.size());
    std::string checksum = CalculateChecksum(message);
    bool encrypted = false;

    // Вычислить fingerprint публичного ключа получателя
    std::string to;
    if (!recipient_public_key_files.empty()) {
        EVP_PKEY* recipient_public_key = LoadKeyFromFile(recipient_public_key_files[0], false); // Предполагается, что есть хотя бы один получатель
        to = GetPublicKeyFingerprint(recipient_public_key);
        EVP_PKEY_free(recipient_public_key);
    } else {
        to = "user<unknown>"; // Если публичный ключ получателя не задан
    }

    std::map<std::string, std::string> data = {
        {"to", to},
        {"len", length},
        {"checksum", checksum},
        {"encrypted", encrypted ? "true" : "false"},
        {"content", message}
    };

    // Подписывание сообщения
    std::vector<unsigned char> signature;
    try {
        signature = SignMessage(message, client_private_key_);
        data["signature"] = std::string(signature.begin(), signature.end());
    } catch (const std::exception& e) {
        std::cerr << "Signing error: " << e.what() << std::endl;
        return;
    }

    if (encrypted) {
        std::vector<unsigned char> encrypted_msg;
        try {
            // Зашифровать сообщение для каждого получателя
            for (const auto& pubkey_file : recipient_public_key_files) {
                EVP_PKEY* public_key = LoadKeyFromFile(pubkey_file, false);
                encrypted_msg = Client::EncryptMessage(message, public_key);
                EVP_PKEY_free(public_key);
                std::array<char, MAX_IP_PACK_SIZE> encrypted_array;
                std::copy(encrypted_msg.begin(), encrypted_msg.end(), encrypted_array.begin());
                write_msgs_.push_back(encrypted_array);
            }
        } catch (const std::exception& e) {
            std::cerr << "Encryption error: " << e.what() << std::endl;
            return;
        }
    } else {
        // Используем сообщение напрямую без шифрования
        std::string packed_message = Message::pack(data);
        std::array<char, MAX_IP_PACK_SIZE> formatted_msg;
        std::copy(packed_message.begin(), packed_message.end(), formatted_msg.begin());
        write_msgs_.push_back(formatted_msg);

        // std::array<char, MAX_IP_PACK_SIZE> plain_array;
        // std::copy(env.begin(), env.end(), plain_array.begin());
        // write_msgs_.push_back(plain_array);

        if (!write_in_progress) {
            boost::asio::async_write(socket_,
                                     boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                                     boost::bind(&Client::WriteHandler, this, _1));
        }
    }
}

void Client::WriteHandler(const boost::system::error_code& error) {
    if (!error) {
        write_msgs_.pop_front();
        if (!write_msgs_.empty()) {
            boost::asio::async_write(socket_,
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

std::vector<unsigned char> Client::EncryptMessage(const std::string& message, EVP_PKEY* public_key) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, nullptr);
    if (!ctx) {
        throw std::runtime_error("Error creating context for encryption");
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error initializing encryption");
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error setting RSA padding");
    }

    size_t outlen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, reinterpret_cast<const unsigned char*>(message.c_str()), message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error determining buffer length for encryption");
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_encrypt(ctx, out.data(), &outlen, reinterpret_cast<const unsigned char*>(message.c_str()), message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error encrypting message");
    }

    EVP_PKEY_CTX_free(ctx);
    return out;
}


std::string Client::DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
        throw std::runtime_error("Error creating context for decryption");
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error initializing decryption");
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error setting RSA padding");
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error determining buffer length for decryption");
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Error decrypting message");
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

std::string Client::GetPublicKeyFingerprint(EVP_PKEY* public_key) {
    unsigned char* der = nullptr;
    int len = i2d_PUBKEY(public_key, &der);
    if (len < 0) {
        throw std::runtime_error("Failed to convert public key to DER format");
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    if (!EVP_Digest(der, len, hash, &hash_len, EVP_sha256(), nullptr)) {
        OPENSSL_free(der);
        throw std::runtime_error("Failed to compute SHA-256 hash of public key");
    }
    OPENSSL_free(der);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}


EVP_PKEY* Client::LoadKeyFromFile(const std::string& key_file, bool is_private, const std::string& password) {
    FILE* fp = fopen(key_file.c_str(), "r");
    if (!fp) {
        std::cerr << "Error opening key file: " << key_file << std::endl;
        return nullptr;
    }

    EVP_PKEY* key = nullptr;
    if (is_private) {
        key = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char*>(password.c_str()));
    } else {
        key = PEM_read_PUBKEY(fp, nullptr, nullptr, nullptr);
    }
    fclose(fp);

    if (!key) {
        std::cerr << "Error loading key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    }

    return key;
}


std::vector<unsigned char> Client::SignMessage(const std::string& message, EVP_PKEY* private_key) {
    // Создаем контекст для подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        throw std::runtime_error("Failed to create EVP_MD_CTX");
    }

    // Инициализируем контекст для подписи с использованием алгоритма SHA-256
    if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error initializing signing: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error updating signing: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    // Определяем размер подписи
    size_t siglen;
    if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error finalizing signing: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
    }

    // Получаем подпись
    std::vector<unsigned char> signature(siglen);
    if (EVP_DigestSignFinal(mdctx, signature.data(), &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        throw std::runtime_error("Error getting signature: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
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
