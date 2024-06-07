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

// void Client::ReadHandler(const boost::system::error_code& error) {
//     std::cout << "\n[[[<<<" << read_msg_.data() << ">>>]]]\n" << std::endl;
//     if (!error) {
//         std::vector<unsigned char> encrypted_msg(read_msg_.begin(), read_msg_.end());
//         try {
//             std::string decrypted_msg = DecryptMessage(encrypted_msg, client_private_key_);
//             std::cout << decrypted_msg << std::endl;
//         } catch (const std::exception& e) {
//             std::cerr << "Decryption error: " << e.what() << std::endl;
//         }

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
        // std::string received_message(read_msg_.data());
        // std::string to =
        //     received_message.substr(received_message.find("to:") + 3,
        //                             received_message.find("|checksum:") - received_message.find("to:") - 3);
        // std::string checksum = received_message.substr(received_message.find("checksum:") + 9,
        //                                                received_message.find("|encrypted:") - received_message.find("checksum:") - 9);
        // std::string encrypted_str = received_message.substr(received_message.find("encrypted:") + 10,
        //                                                     received_message.find("|message:") - received_message.find("encrypted:") - 10);
        // bool encrypted = encrypted_str == "true";
        // std::string message = received_message.substr(received_message.find("|message:") + 9);

        // // if (encrypted) {
        // //     // std::vector<unsigned char> encrypted_msg(read_msg_.begin(), read_msg_.end());
        // //     // try {
        // //     //     // std::string decrypted_msg = Client::DecryptMessage(encrypted_msg, client_private_key_);
        // //     //     // std::string checksum = decrypted_msg.substr(decrypted_msg.size() - SHA256_DIGEST_LENGTH * 2);
        // //     //     // decrypted_msg = decrypted_msg.substr(0, decrypted_msg.size() - SHA256_DIGEST_LENGTH * 2);

        // //     //     // if (VerifyChecksum(decrypted_msg, checksum)) {
        // //     //     //     std::cout << "Message received successfully: " << decrypted_msg << std::endl;
        // //     //     // } else {
        // //     //     //     std::cerr << "Checksum verification failed." << std::endl;
        // //     //     // }

        // //     //     // Используем сообщение напрямую без расшифровки и проверки контрольной суммы
        // //     //     std::cout << "Message received: " << std::string(read_msg_.data()) << std::endl;

        // //     // } catch (const std::exception& e) {
        // //     //     std::cerr << "Decryption error: " << e.what() << std::endl;
        // //     // }
        // // }

        // std::cout << "To: " << to << "\nChecksum: " << checksum << "\nEncrypted: " << (encrypted ? "true" : "false") << "\nMessage: " << message << std::endl;

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
    std::string checksum = CalculateChecksum(message);
    bool encrypted = false;

    // Вычислить fingerprint публичного ключа получателя
    std::string to;
    if (!recipient_public_key_files.empty()) {
        EVP_PKEY* recipient_public_key = LoadKeyFromFile(recipient_public_key_files[0], false); // Предполагается, что есть хотя бы один получатель
        to = "user<" + GetPublicKeyFingerprint(recipient_public_key) + ">";
        EVP_PKEY_free(recipient_public_key);
    } else {
        to = "user<unknown>"; // Если публичный ключ получателя не задан
    }

    std::string env =
        "to:" + to +
        "|checksum:" + checksum +
        "|encrypted:" + (encrypted ? "true" : "false") +
        "|message:" + message;

    // // Зашифровать сообщение для каждого получателя
    // std::vector<unsigned char> encrypted_msg;
    // try {
    //     for (const auto& pubkey_file : recipient_public_key_files) {
    //         FILE* pubkey_fp = fopen(pubkey_file.c_str(), "r");
    //         if (!pubkey_fp) {
    //             std::cerr << "Error opening public key file: " << pubkey_file << std::endl;
    //             continue;
    //         }

    //         EVP_PKEY* public_key = PEM_read_PUBKEY(pubkey_fp, nullptr, nullptr, nullptr);
    //         fclose(pubkey_fp);

    //         if (!public_key) {
    //             std::cerr << "Error loading public key: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
    //             continue;
    //         }

    //         encrypted_msg = Client::EncryptMessage(message, public_key);
    //         EVP_PKEY_free(public_key);

    //         // Конвертировать зашифрованное сообщение обратно в std::array<char, MAX_IP_PACK_SIZE>
    //         if (encrypted_msg.size() > MAX_IP_PACK_SIZE) {
    //             std::cerr << "Encrypted message is too large to fit in the buffer" << std::endl;
    //             continue;
    //         }
    //         std::array<char, MAX_IP_PACK_SIZE> encrypted_array;
    //         std::copy(encrypted_msg.begin(), encrypted_msg.end(), encrypted_array.begin());
    //         write_msgs_.push_back(encrypted_array);
    //     }
    // } catch (const std::exception& e) {
    //     std::cerr << "Encryption error: " << e.what() << std::endl;
    //     return;
    // }

    // Используем сообщение напрямую без шифрования
    std::array<char, MAX_IP_PACK_SIZE> plain_array;
    std::copy(env.begin(), env.end(), plain_array.begin());
    write_msgs_.push_back(plain_array);

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
