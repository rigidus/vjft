// Crypt.cpp
#include "Crypt.hpp"

std::array<unsigned char, HASH_SIZE> Crypt::calcCRC(
    const std::string& message)
{
    std::array<unsigned char, HASH_SIZE> hash = {};
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "Error Calc CRC: Failed to create EVP_MD_CTX" << std::endl;
        return hash;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Error Calc CRC: Failed to initialize digest" << std::endl;
        return hash;
    }

    if (EVP_DigestUpdate(mdctx, message.c_str(), message.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Error Calc CRC: Failed to update digest" << std::endl;
        return hash;
    }

    unsigned int lengthOfHash = 0;
    if (EVP_DigestFinal_ex(mdctx, hash.data(), &lengthOfHash) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "Error Calc CRC: Failed to finalize digest" << std::endl;
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


bool Crypt::verify(
    const std::string& message, const std::array<unsigned char, 512>& signature,
    EVP_PKEY* public_key)
{
    // Создаем контекст для проверки подписи
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "VerifySignature error: Failed to create EVP_MD_CTX" << std::endl;
        return false;
    }

    // Инициализируем контекст для проверки подписи с использованием алгоритма SHA-256
    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error initializing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Обновляем контекст данными сообщения
    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error updating verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Проверяем подпись
    int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    EVP_MD_CTX_free(mdctx);

    if (ret < 0) {
        std::cerr << "VerifySignature error: Error finalizing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    // Возвращаем true, если подпись верна, иначе false
    return ret == 1;
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
        std::cerr << "Decrypt error: Error creating context for decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error initializing decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_chunk.data(), encrypted_chunk.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error determining buffer length for decryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_chunk.data(), encrypted_chunk.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "Decrypt error: Error decrypting chunk" << std::endl;
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

std::optional<std::string> Crypt::DecryptMessage(const std::vector<unsigned char>& encrypted_message, EVP_PKEY* private_key) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(private_key, nullptr);
    if (!ctx) {
        std::cerr << "DecryptMessage error: Error creating context for decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error initializing decryption" << std::endl;
        return std::nullopt;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error setting RSA padding" << std::endl;
        return std::nullopt;
    }

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error determining buffer length for decryption" << std::endl;
        return std::nullopt;
    }

    std::vector<unsigned char> out(outlen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outlen, encrypted_message.data(), encrypted_message.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        std::cerr << "DecryptMessage error: Error decrypting message" << std::endl;
        return std::nullopt;
    }

    EVP_PKEY_CTX_free(ctx);
    return std::string(out.begin(), out.end());
}

bool Crypt::VerifySignature(const std::string& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (mdctx == nullptr) {
        std::cerr << "VerifySignature error: Failed to create EVP_MD_CTX" << std::endl;
        return false;
    }

    if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, public_key) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error initializing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    if (EVP_DigestVerifyUpdate(mdctx, message.c_str(), message.size()) <= 0) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "VerifySignature error: Error updating verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

    int ret = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());
    EVP_MD_CTX_free(mdctx);

    if (ret < 0) {
        std::cerr << "VerifySignature error: Error finalizing verification: "
                  << std::string(ERR_error_string(ERR_get_error(), nullptr))
                  << std::endl;
        return false;
    }

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
#if (DBG > 0)
    std::cout << std::endl;
#endif

    std::vector<unsigned char> envelope;
    envelope.clear();

    // Form random - std::vector<unsigned char> of random length
    // from 0 to 255 and random content, which is necessary to make
    // it impossible to guess the meaning of the message by its length
    // (for example, for short messages like "yes" or "no").
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // random init
    int random_len = std::rand() % 256; // random length
#if (DBG > 0)
    dbg_out_scalar(":random_len", random_len, 1);
#endif
    std::vector<unsigned char> random;
    for (int i = 0; i < random_len; ++i) {
        random.push_back(static_cast<unsigned char>(std::rand() % 256));
    }
#if (DBG > 0)
    dbg_out_vec(":random", random);
#endif

    // Write random_len (1 byte) to envelope
    envelope.push_back(static_cast<unsigned char>(random_len & 0xFF));

    // Write random contents
    envelope.insert(envelope.end(), random.begin(), random.end());

    // Calcuate msg_size
    uint16_t msg_size = static_cast<uint16_t>(msg.size());
#if (DBG > 0)
    dbg_out_scalar(":msg_size", msg_size, 2);
#endif

    // Write msg_zise to envelope, after random
    // (the least significant byte comes first)
    envelope.push_back(static_cast<unsigned char>(msg_size & 0xFF));
    envelope.push_back(static_cast<unsigned char>((msg_size >> 8) & 0xFF));

    // Calculate msg_crc = CRC(msg)
    std::array<unsigned char, HASH_SIZE> msg_crc = Crypt::calcCRC(msg);
#if (DBG > 0)
    std::vector<unsigned char> vcrc;
    vcrc.insert(vcrc.end(), msg_crc.begin(), msg_crc.end());
    dbg_out_vec(":msg_crc", vcrc);
#endif

    // Write msg_crc to envelope, after msg_size
    envelope.insert(envelope.end(), msg_crc.begin(), msg_crc.end());

    // Calculate msg_sizn = sign(msg)
    auto optSign = Crypt::sign(msg, private_key);
    if (!optSign) {
        throw std::runtime_error("Signing message failed");
    }
    std::array<unsigned char, SIG_SIZE> msg_sign = *optSign;

    // Write msg_sign to envelope, after msg_crc
    envelope.insert(envelope.end(), msg_sign.begin(), msg_sign.end());
#if (DBG > 0)
    std::vector<unsigned char> vsign;
    vsign.insert(vsign.end(), msg_sign.begin(), msg_sign.end());
    dbg_out_vec(":msg_sign", vsign);
#endif

    // Write msg to envelope, after msg_sign
    envelope.insert(envelope.end(), msg.begin(), msg.end());
#if (DBG > 0)
    std::vector<unsigned char> vmsg;
    vmsg.insert(vmsg.end(), msg.begin(), msg.end());
    dbg_out_vec(":msg", vmsg);
#endif

    // dbg envelope
#if (DBG > 0)
    dbg_out_vec(":envelope", envelope);
#endif

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
#if (DBG > 0)
        dbg_out_vec(":chunk", chunk);
#endif
        // ENCRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_enc_chunk =
            Crypt::Encrypt(chunk, public_key);
        if (!(opt_enc_chunk)) {
            std::cerr << "Error: Encryption Chunk Failed" << std::endl;
            throw "qwqewqewqe";
            throw std::runtime_error("Encryption Chunk Failed");
        }
        // SAVE ENCRYPT CHUNK
        std::vector<unsigned char> enc_chunk = *opt_enc_chunk;
        enc_chunks.push_back(enc_chunk);
#if (DBG > 0)
        dbg_out_vec(":enc_chunk", enc_chunk);
#endif
    }


    // Forming a packet to be transmitted over the network
    std::vector<unsigned char> pack;
    pack.clear();

    // First we write the two-byte envelope size in chunks
    // (the least significant byte comes first)
    uint16_t envelope_size = static_cast<uint16_t>(enc_chunks.size());
#if (DBG > 0)
    dbg_out_scalar(":envelope_size", envelope_size, 2);
#endif
    pack.push_back(static_cast<unsigned char>(envelope_size & 0xFF));
    pack.push_back(static_cast<unsigned char>((envelope_size >> 8) & 0xFF));

    // After write all enc_chunks
    for (const auto& enc_chunk : enc_chunks) {
        pack.insert(pack.end(), enc_chunk.begin(), enc_chunk.end());
    }

    /** Finally, Pack is:
        +-------------------------+
        | envelope_size_in_chunks | 2 bytes
        +-------------------------+
        | enc_chunks              |
        |       ....              |
        +-------------------------+
    */

#if (DBG > 0)
    dbg_out_vec(":pack", pack);
#endif
    return pack;
}


std::string Crypt::decipher(
    EVP_PKEY* private_key, EVP_PKEY* public_key, std::vector<unsigned char> pack)
{
#if (DBG > 0)
    std::cout << "--------------------" <<std::endl;
    dbg_out_vec("|pack", pack);
#endif

    // Extract envelope_size from pack
    uint16_t envelope_size =
        static_cast<uint16_t>(pack[0]) | (static_cast<uint16_t>(pack[1]) << 8);
#if (DBG > 0)
    dbg_out_scalar("|envelope_size", envelope_size, 2);
#endif

    // Extract enc_chunks from pack
    std::vector<std::vector<unsigned char>> enc_chunks;
    auto it = pack.begin() + 2;
    for (size_t i = 0; i < envelope_size; ++i) {
        if (std::distance(it, pack.end()) <static_cast<ptrdiff_t>(ENC_CHUNK_SIZE)) {
            throw std::runtime_error("Not enough data to read chunk");
        }
        enc_chunks.emplace_back(it, it + ENC_CHUNK_SIZE);
        it += ENC_CHUNK_SIZE;
    }

    // Decrypt every enc_chunk with priv key and write to chunks
    std::vector<std::vector<unsigned char>> chunks;
    // size_t bytes_left = size_of_msg;
    for (std::vector<unsigned char> chunk : enc_chunks) {
#if (DBG > 0)
        // dbg encrypted chunk
        dbg_out_vec("|enc-chunk", chunk);
#endif
        // DECRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_dec_chunk =
            Crypt::Decrypt(chunk, private_key);
        if (!opt_dec_chunk) {
            std::cerr << "Error: Decryption Chunk Failed" << std::endl;
            throw std::runtime_error("Decryption Chunk Failed");
        }
        // SAVE DECRYPTED CHUNK
        std::vector<unsigned char> dec_chunk = *opt_dec_chunk;
        // size corrections
        dec_chunk.resize(CHUNK_SIZE);
        // SAVE RESULT
        chunks.push_back(dec_chunk);
#if (DBG > 0)
        // dbg decrypted chunk
        dbg_out_vec("|dec-chunk", dec_chunk);
#endif
    }

    // Form envelope
    std::vector<unsigned char> envelope;
    for (const auto& enc_chunk : chunks) {
        envelope.insert(envelope.end(), enc_chunk.begin(), enc_chunk.end());
    }

#if (DBG > 0)
    dbg_out_vec("|envelope", envelope);
#endif

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
#if (DBG > 0)
    dbg_out_scalar("|random_len", random_len, 1);
#endif

    // Calculate ioffset to next field of envelope
    size_t offset = random_len + 1;

    // Extract msg_size
    uint16_t msg_size =
        static_cast<uint16_t>(envelope[offset]) |
        (static_cast<uint16_t>(envelope[offset+1]) << 8);
    offset += 2;
#if (DBG > 0)
    dbg_out_scalar("|msg_size", msg_size, 2);
#endif

    // Extract msg_crc
    std::vector<unsigned char> msg_crc;
    msg_crc.insert(
        msg_crc.end(), envelope.begin()+offset, envelope.begin()+offset+HASH_SIZE);
    offset += HASH_SIZE;
#if (DBG > 0)
    std::vector<unsigned char> vcrc;
    vcrc.insert(vcrc.end(), msg_crc.begin(), msg_crc.end());
    dbg_out_vec("|msg_crc", vcrc);
#endif

    // Extract msg_sign
    std::vector<unsigned char> msg_sign;
    msg_sign.insert(
        msg_sign.end(), envelope.begin()+offset, envelope.begin()+offset+SIG_SIZE);
    offset += SIG_SIZE;
#if (DBG > 0)
    std::vector<unsigned char> vsign;
    vsign.insert(vsign.end(), msg_sign.begin(), msg_sign.end());
    dbg_out_vec("|msg_sign", vsign);
#endif

    // Extract msg
    std::string msg(envelope.begin()+offset, envelope.begin()+offset+msg_size);
#if (DBG > 0)
    std::cout << "msg: [" << msg << "]" << std::endl;
#endif

    // CHECK SIGNATURE
    if (!Crypt::verify(msg, *reinterpret_cast<std::array<unsigned char, SIG_SIZE>*>(
                           msg_sign.data()), public_key))
    {
        std::cerr << "Error: Message Signature Verification Failed" << std::endl;
        return "";
    } else {
        std::cout << "Message Signature Verification Ok" << std::endl;
    }

    // CHECK CHECKSUM
    if (!Crypt::verifyChecksum(msg,
                               *reinterpret_cast<std::array<unsigned char, HASH_SIZE>*>(
                                   msg_crc.data())))
    {
        std::cerr << "Error: Message CRC Verification Failed" << std::endl;
        return "";
    } else {
        std::cout << "Message CRC Verification Ok" << std::endl;
    }

    return msg;
}
