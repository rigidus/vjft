// test_crypto.cpp

#include "test_crypto.hpp"

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
std::vector<unsigned char> encipher(
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


std::string decipher(
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

bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key, std::string msg)
{
    bool result = false;

    std::vector<unsigned char> cipher = encipher(private_key, public_key, msg);

    std::string new_msg = decipher(private_key, public_key, cipher);

    result = (msg == new_msg);

    return result;
}

int main() {
    std::string private_key_file = "client_private_key.pem";
    std::string public_key_file = "client_public_key.pem";
    std::string password;

    std::cout << "Enter password for private key: ";
    std::cin >> password;

    EVP_PKEY* private_key = Client::LoadKeyFromFile(private_key_file, true, password);
    EVP_PKEY* public_key = Client::LoadKeyFromFile(public_key_file, false);

    if (!private_key || !public_key) {
        std::cerr << "\nTest Full Sequence: Error |: No key files" << std::endl;
        return 1;
    }

    std::string message = "Lorem Ipsum - это текст-рыба, часто используемый в печати и вэб-дизайне. Lorem Ipsum является стандартной рыбой для текстов на латинице с начала XVI века. В то время некий безымянный печатник создал большую коллекцию размеров и форм шрифтов, используя Lorem Ipsum для распечатки образцов. Lorem Ipsum не только успешно пережил без заметных изменений пять веков, но и перешагнул в электронный дизайн. Его популяризации в новое время послужили публикация листов Letraset с образцами Lorem Ipsum в 60-х годах и, в более недавнее время, программы электронной вёрстки типа Aldus PageMaker, в шаблонах которых используется Lorem Ipsum. Давно выяснено, что при оценке дизайна и композиции читаемый текст мешает сосредоточиться. Lorem Ipsum используют потому, что тот обеспечивает более или менее стандартное заполнение шаблона, а также реальное распределение букв и пробелов в абзацах, которое не получается при простой дубликации. Многие программы электронной вёрстки и редакторы HTML используют Lorem Ipsum в качестве текста по умолчанию, так что поиск по ключевым словам lorem ipsum сразу показывает, как много веб-страниц всё ещё дожидаются своего настоящего рождения. За прошедшие годы текст Lorem Ipsum получил много версий. Некоторые версии появились по ошибке, некоторые - намеренно (например, юмористические варианты). Многие думают, что Lorem Ipsum - взятый с потолка псевдо-латинский набор слов, но это не совсем так. Его корни уходят в один фрагмент классической латыни 45 года н.э., то есть более двух тысячелетий назад. Ричард МакКлинток, профессор латыни из колледжа Hampden-Sydney, штат Вирджиния, взял одно из самых странных слов в Lorem Ipsum, consectetur, и занялся его поисками в классической латинской литературе. В результате он нашёл неоспоримый первоисточник Lorem Ipsum в разделах 1.10.32 и 1.10.33 книги de Finibus Bonorum et Malorum (О пределах добра и зла), написанной Цицероном в 45 году н.э. Этот трактат по теории этики был очень популярен в эпоху Возрождения. Первая строка Lorem Ipsum, Lorem ipsum dolor sit amet.., происходит от одной из строк в разделе 1.10.32. Классический текст Lorem Ipsum, используемый с XVI века, приведён ниже. Также даны разделы 1.10.32 и 1.10.33 de Finibus Bonorum et Malorum Цицерона и их английский перевод, сделанный H. Rackham, 1914 год.";

    bool full_sequence_result = TestFullSequence(private_key, public_key, message);

    std::cout << "\nTest Full Sequence: "
    << (full_sequence_result ? "PASSED" : "FAILED") << std::endl;

    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);

    return 0;
}
