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
    client_private_key_ = LoadPrivateKey(client_private_key_file, password);

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

// void Client::ReadHandler(const boost::system::error_code& error) {
//     std::cout << read_msg_.data() << std::endl;
//     if (!error) {
//         boost::asio::async_read(socket_,
//                                 boost::asio::buffer(read_msg_, read_msg_.size()),
//                                 boost::bind(&Client::ReadHandler, this, _1));
//     } else {
//         CloseImpl();
//     }
// }

void Client::ReadHandler(const boost::system::error_code& error) {
    std::cout << "\n[[[<<<" << read_msg_.data() << ">>>]]]\n" << std::endl;
    if (!error) {
        std::vector<unsigned char> encrypted_msg(read_msg_.begin(), read_msg_.end());
        try {
            std::string decrypted_msg = DecryptMessage(encrypted_msg, client_private_key_);
            std::cout << decrypted_msg << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Decryption error: " << e.what() << std::endl;
        }

        boost::asio::async_read(socket_,
                                boost::asio::buffer(read_msg_, read_msg_.size()),
                                boost::bind(&Client::ReadHandler, this, _1));
    } else {
        CloseImpl();
    }
}


// void Client::WriteImpl(std::array<char, MAX_IP_PACK_SIZE> msg) {
//     bool write_in_progress = !write_msgs_.empty();
//     write_msgs_.push_back(msg);
//     if (!write_in_progress) {
//         boost::asio::async_write(socket_,
//                                  boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
//                                  boost::bind(&Client::WriteHandler, this, _1));
//     }
// }

void Client::WriteImpl(std::array<char, MAX_IP_PACK_SIZE> msg) {
    bool write_in_progress = !write_msgs_.empty();

    // Зашифровать сообщение для каждого получателя
    std::vector<unsigned char> encrypted_msg;
    try {
        for (const auto& pubkey_file : recipient_public_key_files) {
            FILE* pubkey_fp = fopen(pubkey_file.c_str(), "r");
            if (!pubkey_fp) {
                std::cerr << "Error opening public key file: " << pubkey_file << std::endl;
                continue;
            }

            EVP_PKEY* public_key = PEM_read_PUBKEY(pubkey_fp, nullptr, nullptr, nullptr);
            fclose(pubkey_fp);

            if (!public_key) {
                std::cerr << "Error loading public key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
                continue;
            }

            std::string message(msg.data());
            encrypted_msg = EncryptMessage(message, public_key);

            EVP_PKEY_free(public_key);

            // Конвертировать зашифрованное сообщение обратно в std::array<char, MAX_IP_PACK_SIZE>
            if (encrypted_msg.size() > MAX_IP_PACK_SIZE) {
                std::cerr << "Encrypted message is too large to fit in the buffer" << std::endl;
                continue;
            }
            std::array<char, MAX_IP_PACK_SIZE> encrypted_array;
            std::copy(encrypted_msg.begin(), encrypted_msg.end(), encrypted_array.begin());
            write_msgs_.push_back(encrypted_array);
        }
    } catch (const std::exception& e) {
        std::cerr << "Encryption error: " << e.what() << std::endl;
        return;
    }

    if (!write_in_progress) {
        boost::asio::async_write(socket_,
                                 boost::asio::buffer(write_msgs_.front(), write_msgs_.front().size()),
                                 boost::bind(&Client::WriteHandler, this, _1));
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

EVP_PKEY* Client::LoadPrivateKey(const std::string& key_file, const std::string& password) {
    FILE* fp = fopen(key_file.c_str(), "r");
    if (!fp) {
        std::cerr << "Error opening private key file: " << key_file << std::endl;
        return nullptr;
    }

    EVP_PKEY* pkey = nullptr;
    pkey = PEM_read_PrivateKey(fp, nullptr, nullptr, const_cast<char*>(password.c_str()));
    fclose(fp);

    if (!pkey) {
        std::cerr << "Error loading private key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    }

    return pkey;
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
