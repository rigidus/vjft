// Crypt.cpp
#include "Crypt.hpp"

EVP_PKEY* Crypt::LoadKeyFromFile(const std::string& key_file, bool is_private,
                                 const std::string& password)
{
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

std::string Crypt::GetPubKeyFingerprint(EVP_PKEY* public_key) {
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

std::array<unsigned char, HASH_SIZE> Crypt::calcCRC(
    const std::string& message)
{
    std::array<unsigned char, HASH_SIZE> hash = {};
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << ":> Crypt::calcCRC(): Error: Failed to create EVP_MD_CTX"
                  << std::endl;
        return hash;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << ":> Crypt::calcCRC(): Error: Failed to initialize digest"
                  << std::endl;
        return hash;
    }

    if (EVP_DigestUpdate(mdctx, message.c_str(), message.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << ":> Crypt::calcCRC(): Error: Failed to update digest"
                  << std::endl;
        return hash;
    }

    unsigned int lengthOfHash = 0;
    if (EVP_DigestFinal_ex(mdctx, hash.data(), &lengthOfHash) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << ":> Crypt::calcCRC(): Error: Failed to finalize digest"
                  << std::endl;
        return hash;
    }

    EVP_MD_CTX_free(mdctx);
    return hash;
}


bool Crypt::verifyChecksum(const std::string& message,
                           const std::array<unsigned char, HASH_SIZE>& checksum)
{
    std::array<unsigned char, HASH_SIZE> calculatedChecksum = calcCRC(message);
    return calculatedChecksum == checksum;
}


std::optional<std::array<unsigned char, SIG_SIZE>> Crypt::sign(
    const std::string& message, EVP_PKEY* private_key)
{
    std::array<unsigned char, 512> signature = {};
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "Signing error: Failed to create EVP_MD_CTX" << std::endl;
        return std::nullopt;
    }

    if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error initializing signing: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return std::nullopt;
    }

    if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error updating signing: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    size_t siglen;
    if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error finalizing signing: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    if (siglen != signature.size()) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Unexpected signature size" << std::endl;
        return std::nullopt;
    }

    if (EVP_DigestSignFinal(mdctx, signature.data(), &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Signing error: Error getting signature: " <<
            std::string(ERR_error_string(ERR_get_error(), nullptr)) << std::endl;
        return std::nullopt;
    }

    EVP_MD_CTX_free(mdctx);

    return signature;
}


std::optional<std::vector<unsigned char>> Crypt::Encrypt(
    const std::vector<unsigned char>& packed, EVP_PKEY* public_key)
{
    size_t encrypted_length = 0;
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(public_key, nullptr);
    if (!ctx) {
        std::cerr << "Encrypt error: Error creating context for encryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error initializing encryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_encrypt(ctx, nullptr, &encrypted_length, packed.data(), packed.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error determining buffer length for encryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> encrypted(encrypted_length);
    if (EVP_PKEY_encrypt(ctx, encrypted.data(), &encrypted_length, packed.data(), packed.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Encrypt error: Error encrypting message" << std::endl;
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return encrypted;
}


std::optional<std::vector<unsigned char>> Crypt::Decrypt(
    const std::vector<unsigned char>& encrypted_chunk, EVP_PKEY* private_key)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
#if (DBG > 0)
        std::cerr << "Decrypt error: Error creating context for decryption" << std::endl;
#endif
        return std::nullopt;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
#if (DBG > 0)
        std::cerr << "Decrypt error: Error initializing decryption" << std::endl;
#endif
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
#if (DBG > 0)
        std::cerr << "Decrypt error: Error setting RSA padding" << std::endl;
#endif
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_chunk.data(),
                         encrypted_chunk.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
#if (DBG > 0)
        std::cerr << "Decrypt error: Error determining buffer length for decryption"
                  << std::endl;
#endif
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_chunk.data(),
                         encrypted_chunk.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
#if (DBG > 0)
        std::cerr << "Decrypt error: Error decrypting chunk" << std::endl;
#endif
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return out;
}


std::string Crypt::Base64Encode(const std::vector<unsigned char>& buffer) {
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

std::vector<unsigned char> Crypt::Base64Decode(const std::string& encoded) {
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

std::optional<std::string> Crypt::DecryptMessage(
    const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key)
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
        std::cerr << "!> Crypt::DecryptMessage(): Error creating context for decryption"
                  << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "!> Crypt::DecryptMessage():  Error initializing decryption"
                  << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "!> Crypt::DecryptMessage(): Error setting RSA padding"
                  << std::endl;
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_message.data(),
                         encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "!> Crypt::DecryptMessage(): Error determining buffer "
                  << "length for decryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_message.data(),
                         encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "!> Crypt::DecryptMessage(): Error decrypting message"
                  << std::endl;
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return std::string(out.begin(), out.end());
}


std::optional<std::vector<unsigned char>> Crypt::SignMsg(const std::string& message,
                                                         EVP_PKEY* private_key)
{
    // Создаем контекст для подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "!> Client::SignMsg(): Error: Failed to create EVP_MD_CTX"
                  << std::endl;
        return std::nullopt;
    }

    // Инициализируем контекст для подписи с использованием алгоритма SHA-256
    if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, private_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "!> Client::SignMsg(): Error initializing signing: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return std::nullopt;
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestSignUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "!> Client::SignMsg(): Error updating signing: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return std::nullopt;
    }

    // Определяем размер подписи
    size_t siglen;
    if (EVP_DigestSignFinal(mdctx, nullptr, &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "!> Client::SignMsg(): Error finalizing signing: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return std::nullopt;
    }

    // Получаем подпись
    std::vector<unsigned char> signature(siglen);
    if (EVP_DigestSignFinal(mdctx, signature.data(), &siglen) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "!> Client::SignMsg(): Error: Error getting signature: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return std::nullopt;
    }

    // Освобождаем контекст
    EVP_MD_CTX_free(mdctx);

    // Обрезаем вектор до актуального размера подписи, если это необходимо
    std::cout << ":> Client::SignMsg(): siglen:" << siglen << std::endl;
    signature.resize(siglen);

    // Возвращаем подпись
    return signature;
}


bool Crypt::VerifySignature(const std::string& message,
                            const std::vector<unsigned char>& signature,
                            EVP_PKEY* public_key)
{
    // Создаем контекст для проверки подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "!> Crypt::VerifySignature(): Error: Failed to create EVP_MD_CTX"
                  << std::endl;
        return false;
    }

    // Инициализируем контекст для проверки подписи с использованием алгоритма SHA-256
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "!> Crypt::VerifySignature(): Error initializing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "!> Crypt::VerifySignature(): Error updating verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Проверяем подпись
    int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    EVP_MD_CTX_free(mdctx);

    if (ret < 0) {
        std::cerr << "!> Crypt::VerifySignature(): Error finalizing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Возвращаем true, если подпись верна, иначе false
    return ret == 1;
}


/**
   Encryption takes place in two steps: first, an envelope is
   formed that structurally represents the message:
   +---------------+
   | random_len    | 1 byte
   +---------------+
   | random...     |
   |   (0-0xFF)    | 0..0xFF bytes
   +---------------+
   | msg_size      | 2 bytes
   +---------------+
   | msg_crc       | 32 bytes
   +---------------+
   | msg_sign      | 512 bytes
   +---------------+
   | msg           | variable bytes
   +---------------+

   Then the envelope is divided into chunks, each of which is
   encrypted. The chunks have a standardized size, the number
   of chunks is stored in two bytes before the set of chunks:

   +-------------------------+
   | envelope_size_in_chunks | 2 bytes
   +-------------------------+
   | enc_chunks              |
   |       ....              |
   +-------------------------+

   Thus, after receiving the first two bytes, the server or
   proxy can work with the rest of the encrypted message as
   a whole, even if the different messages are of different
   lengths (in any case a multiple of the chunk size).

   Note that the chunks before encryption are 255 bytes, and
   the encrypted chunks are 512 bytes (due to the encryption
   method used).
*/
std::vector<unsigned char> Crypt::encipher(
    EVP_PKEY* private_key, EVP_PKEY* public_key, std::string msg)
{
    std::vector<unsigned char> envelope;
    envelope.clear();

    // Form random - std::vector<unsigned char> of random length
    // from 0 to 255 and random content, which is necessary to make
    // it impossible to guess the meaning of the message by its length
    // (for example, for short messages like "yes" or "no").
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // random init
    int random_len = std::rand() % 256; // random length
    random_len = 0;
    LOG_HEX("random_len", random_len, 1);
    std::vector<unsigned char> random;
    for (int i = 0; i < random_len; ++i) {
        random.push_back(static_cast<unsigned char>(std::rand() % 256));
    }
    LOG_VEC("random", random);

    // -> Write random_len (1 byte) to envelope
    envelope.push_back(static_cast<unsigned char>(random_len & 0xFF));

    // -> Write random contents
    envelope.insert(envelope.end(), random.begin(), random.end());

    // Calcuate msg_size
    uint16_t msg_size = static_cast<uint16_t>(msg.size());
    LOG_HEX("original msg size in hex", msg_size, 2);

    // -> Write msg_zise to envelope, after random
    // (the least significant byte comes first)
    envelope.push_back(static_cast<unsigned char>(msg_size & 0xFF));
    envelope.push_back(static_cast<unsigned char>((msg_size >> 8) & 0xFF));

    // Calculate msg_crc = CRC(msg)
    std::array<unsigned char, HASH_SIZE> msg_crc = Crypt::calcCRC(msg);

    // Debug print CRC(msg)
    std::vector<unsigned char> vcrc;
    vcrc.insert(vcrc.end(), msg_crc.begin(), msg_crc.end());
    LOG_VEC("msg_crc", vcrc);

    // -> Write msg_crc to envelope, after msg_size
    envelope.insert(envelope.end(), msg_crc.begin(), msg_crc.end());

    // Calculate msg_sign = sign(msg)
    auto optSign = Crypt::sign(msg, private_key);
    if (!optSign) {
        throw std::runtime_error("Signing message failed");
    }
    std::array<unsigned char, SIG_SIZE> msg_sign = *optSign;

    // Debug print SIGN(msg)
    std::vector<unsigned char> vsign;
    vsign.insert(vsign.end(), msg_sign.begin(), msg_sign.end());
    LOG_VEC("msg_sign", vsign);

    // -> Write msg_sign to envelope, after msg_crc
    envelope.insert(envelope.end(), msg_sign.begin(), msg_sign.end());

    // -> Write msg to envelope, after msg_sign
    envelope.insert(envelope.end(), msg.begin(), msg.end());

    // Debug print msg
    std::vector<unsigned char> vmsg;
    vmsg.insert(vmsg.end(), msg.begin(), msg.end());
    LOG_VEC("msg in vector", vmsg);

    // Calcuate envelope size
    uint16_t envelope_size = static_cast<uint16_t>(envelope.size());

    // Debug print envelope size
    LOG_HEX("envelope size in hex", envelope_size, 2);

    // Debug print envelope
    LOG_VEC(":envelope", envelope);


    /** Finally Envelope:
        +---------------+
        | random_len    | 1 byte
        +---------------+
        | random...     |
        |   (0-0xFF)    | 0..0xFF bytes
        +---------------+
        | msg_size      | 2 bytes
        +---------------+
        | msg_crc       | 32 bytes
        +---------------+
        | msg_sign      | 512 bytes
        +---------------+
        | msg           | variable bytes
        +---------------+
    */

    // Split envelope to chunks
    std::vector<std::vector<unsigned char>> chunks = split_vec(envelope, CHUNK_SIZE);

    // Encrypt every chunk with pubkey and wrute to enc_chunks
    std::vector<std::vector<unsigned char>> enc_chunks;
    for (const auto& chunk : chunks) {
        // Debug print chunk
        LOG_VEC("chunk", chunk);
        // ENCRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_enc_chunk =
            Crypt::Encrypt(chunk, public_key);
        if (!(opt_enc_chunk)) {
            LOG_TXT("Error: Encryption Chunk Failed");
            throw std::runtime_error("Encryption Chunk Failed");
        }
        // SAVE ENCRYPT CHUNK
        std::vector<unsigned char> enc_chunk = *opt_enc_chunk;
        enc_chunks.push_back(enc_chunk);
        // Debug encrypted chunk
        LOG_VEC("enc_chunk", enc_chunk);
    }

    // Forming a packet to be transmitted over the network
    std::vector<unsigned char> pack;
    pack.clear();

    // First we write the two-byte envelope size in chunks
    // (the least significant byte comes first)
    uint16_t envelope_chunk_size = static_cast<uint16_t>(enc_chunks.size());
    pack.push_back(static_cast<unsigned char>(envelope_chunk_size & 0xFF));
    pack.push_back(static_cast<unsigned char>((envelope_chunk_size >> 8) & 0xFF));

    // Debug print pack
    LOG_VEC("----Pack before inserting encrypted chunks", pack);

    // Debug print envelope chunk size
    LOG_HEX("envelope_chunk_size in hex", envelope_chunk_size, 2);

    // After write all enc_chunks
    for (const auto& enc_chunk : enc_chunks) {
        pack.insert(pack.end(), enc_chunk.begin(), enc_chunk.end());
    }

    /** Finally, Pack is:
        +---------------------+
        | envelope_chunk_size | 2 bytes
        +---------------------+
        | enc_chunks          |
        |       ....          |
        +---------------------+
    */

    // Debug print pack
    LOG_VEC("----Pack", pack);

    // Debug print pack size
    uint16_t pack_size = static_cast<uint16_t>(pack.size());
    LOG_HEX("pack_size in hex", pack_size, 2);

    // Return
    return pack;
}


std::string Crypt::decipher (EVP_PKEY* private_key, EVP_PKEY* public_key,
                             std::vector<unsigned char> pack)
{
    LOG_VEC("----Pack", pack);

    // Extract envelope_size from pack
    uint16_t envelope_chunk_size =
        static_cast<uint16_t>(pack[0]) | (static_cast<uint16_t>(pack[1]) << 8);

    // Debug print envelope chunk size
    LOG_HEX("Envelope chunk size", envelope_chunk_size, 2);

    // Extract enc_chunks from pack
    std::vector<std::vector<unsigned char>> enc_chunks;
    auto it = pack.begin() + 2;
    for (size_t i = 0; i < envelope_chunk_size; ++i) {
        if (std::distance(it, pack.end()) <static_cast<ptrdiff_t>(ENC_CHUNK_SIZE)) {
            throw std::runtime_error("Not enough data to read chunk");
        }
        enc_chunks.emplace_back(it, it + ENC_CHUNK_SIZE);
        it += ENC_CHUNK_SIZE;
    }

    // Decrypt every enc_chunk with privkey and write to chunks
    std::vector<std::vector<unsigned char>> chunks;
    // size_t bytes_left = size_of_msg;
    for (std::vector<unsigned char> chunk : enc_chunks) {
        // Debug print encrypted chunk
        LOG_VEC("enc-chunk", chunk);
        // DECRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_dec_chunk =
            Crypt::Decrypt(chunk, private_key);
        if (!opt_dec_chunk) {
            LOG_TXT("Error: Decryption Chunk Failed");
            return "";
        }
        // SAVE DECRYPTED CHUNK
        std::vector<unsigned char> dec_chunk = *opt_dec_chunk;
        // size corrections
        dec_chunk.resize(CHUNK_SIZE);
        // SAVE RESULT
        chunks.push_back(dec_chunk);

        // Debug print decrypted chunk
        LOG_VEC("dec-chunk", dec_chunk);
    }

    // Form envelope
    std::vector<unsigned char> envelope;
    for (const auto& enc_chunk : chunks) {
        envelope.insert(envelope.end(), enc_chunk.begin(), enc_chunk.end());
    }

    // Debug print Envelope
    LOG_VEC("envelope", envelope);

    /*
      +---------------+
      | random_len    | 1 byte
      +---------------+
      | random...     |
      |   (0-0xFF)    | 0..0xFF bytes
      +---------------+
      | msg_size      | 2 bytes
      +---------------+
      | msg_crc       | 32 bytes
      +---------------+
      | msg_sign      | 512 bytes
      +---------------+
      | msg           | variable bytes
      +---------------+
    */

    // Extract random_len
    uint8_t random_len = static_cast<uint8_t>(envelope[0]);

    // Debug print random length
    LOG_HEX("random_len", random_len, 1);

    // Calculate ioffset to next field of envelope
    size_t offset = random_len + 1;

    // Extract msg_size
    uint16_t msg_size =
        static_cast<uint16_t>(envelope[offset]) |
        (static_cast<uint16_t>(envelope[offset+1]) << 8);
    offset += 2;

    // Debug print msg size
    LOG_HEX("msg_size", msg_size, 2);

    // Extract msg_crc
    std::vector<unsigned char> msg_crc;
    msg_crc.insert(
        msg_crc.end(), envelope.begin()+offset, envelope.begin()+offset+HASH_SIZE);
    offset += HASH_SIZE;

    // Debug print msg_crc
    std::vector<unsigned char> vcrc;
    vcrc.insert(vcrc.end(), msg_crc.begin(), msg_crc.end());
    LOG_VEC("msg_crc", vcrc);

    // Extract msg_sign
    std::vector<unsigned char> msg_sign;
    msg_sign.insert(
        msg_sign.end(), envelope.begin()+offset, envelope.begin()+offset+SIG_SIZE);
    offset += SIG_SIZE;

    // Debug print msg_sign
    std::vector<unsigned char> vsign;
    vsign.insert(vsign.end(), msg_sign.begin(), msg_sign.end());
    LOG_VEC("msg_sign", vsign);

    // Extract msg
    std::string msg(envelope.begin()+offset, envelope.begin()+offset+msg_size);

    // Debug print msg
    LOG_TXT("msg: [" << msg << "]");

    // CHECK SIGNATURE
    if (!Crypt::VerifySignature(msg, msg_sign, public_key))
    {
        LOG_TXT("Message Signature Verification Failed");
        return "";
    } else {
        LOG_TXT("Message Signature Verification Ok");
    }

    // CHECK CHECKSUM
    if (!Crypt::verifyChecksum(
            msg,
            *reinterpret_cast<std::array<unsigned char, HASH_SIZE>*>(msg_crc.data())))
    {
        LOG_TXT("Error: Message CRC Verification Failed");
        return "";
    } else {
        LOG_TXT("Message CRC Verification Ok");
    }

    return msg;
}
