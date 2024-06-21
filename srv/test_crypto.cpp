// test_crypto.cpp
#include "test_crypto.hpp"
#include <openssl/err.h>

#define CHUNK_SIZE 255
#define ENC_CHUNK_SIZE 512

struct MsgRec {
    std::array<unsigned char, HASH_SIZE>        crc_of_msg;     // 32
    uint16_t                                    size_of_msg;    // 2
    std::array<unsigned char, 512>              sign_of_msg;    // 512
    std::string                                 msg;            // size_of_msg
    std::vector<std::vector<unsigned char>>     chunks;
    uint8_t                                     random_cnt;
    std::vector<unsigned char>                  random;
    std::vector<std::vector<unsigned char>>     enc_chunks;
    uint16_t                                    size_of_envelope;
    std::vector<unsigned char>                  envelope;
};

void dbg_out_vec(std::string dbgmsg, std::vector<unsigned char> vec) {
    std::cout << dbgmsg << ": ";
    for (const auto& byte : vec) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl; // Возвращаемся к десятичному формату
}

void dbg_out_scalar(std::string dbgmsg, size_t value, size_t cnt) {
    std::cout << dbgmsg << ": ";
    for (size_t i = 0; i < cnt; ++i) {
        unsigned char byte = (value >> (i * 8)) & 0xFF;
        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<int>(byte);
    }
    std::cout << std::dec << std::endl; // Возвращаемся к десятичному формату
}

std::vector<std::vector<unsigned char>> splitVec(
    const std::vector<unsigned char>& input, int size)
{
    std::vector<std::vector<unsigned char>> result;
    size_t length = input.size();
    for (size_t i = 0; i < length; i += size) {
        size_t currentChunkSize = std::min(size_t(size), length - i);
        std::vector<unsigned char> chunk(
            input.begin() + i, input.begin() + i + currentChunkSize);
        result.push_back(chunk);
    }
    return result;
}

std::vector<unsigned char> mergeVec(
    const std::vector<std::vector<unsigned char>>& input)
{
    std::vector<unsigned char> result;
    for (const auto& chunk : input) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    return result;
}


std::vector<std::vector<unsigned char>> dec(
    const std::vector<std::vector<unsigned char>> enc_chunks,
    EVP_PKEY* private_key)
{
    std::vector<std::vector<unsigned char>> result;
    // size_t bytes_left = size_of_msg;
    for (std::vector<unsigned char> chunk : enc_chunks) {
        // dbg encrypted chunk
        // dbg_out_vec("enc-chunk", chunk);
        // DECRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_dec_chunk =
            Crypt::Decrypt(chunk, private_key);
        if (!opt_dec_chunk) {
            std::cerr << "Error: Decryption Chunk Failed" << std::endl;
            return {};
        }
        // SAVE DECRYPTED CHUNK
        std::vector<unsigned char> dec_chunk = *opt_dec_chunk;
        // corrections
        // if (bytes_left < 255) {
        //     dec_chunk.resize(bytes_left);
        //     bytes_left = 0;
        // } else {
        //     bytes_left -= 255;
        //     dec_chunk.resize(255);
        // }
        // dbg decrypted chunk
        // dbg_out_vec("enc-chunk", dec_chunk);
        // PUSH RESULT
        result.push_back(dec_chunk);
    }
    return result;
}





std::vector<unsigned char> encipher(
    EVP_PKEY* private_key, EVP_PKEY* public_key, std::string msg)
{
    std::cout << std::endl;

    // Envelope - это конверт, содержащий сообщение и вспомогательную информацию,
    // который будет разбит на chunks а потом каждый chunk будет зашифрован.
    std::vector<unsigned char> envelope;
    envelope.clear();

    // TODO: Формируем random - std::vector<unsigned char> случайной
    // длины от 0 до 255 и случайного содержимого, которое необходимо,
    // чтобы по длине сообщения было нельзя догадаться о его смысле
    // (к примеру, для коротких сообщения "да" или "нет").
    std::srand(static_cast<unsigned int>(std::time(nullptr))); // random init
    int random_len = std::rand() % 256; // random length
    dbg_out_scalar(":random_len", random_len, 1);
    std::vector<unsigned char> random;
    for (int i = 0; i < random_len; ++i) {
        random.push_back(static_cast<unsigned char>(std::rand() % 256));
    }
    // dbg_out_vec(":random", random);

    // Записываем в envelope 1 байт random_len
    envelope.push_back(static_cast<unsigned char>(random_len & 0xFF));

    // Записываем в envelope содержимое random
    envelope.insert(envelope.end(), random.begin(), random.end());

    // Вычисляем размер сообщения msg
    uint16_t msg_size = static_cast<uint16_t>(msg.size());
    dbg_out_scalar(":msg_size", msg_size, 2);

    // Записываем msg_size сразу после random, младший байт первым
    envelope.push_back(static_cast<unsigned char>(msg_size & 0xFF));
    envelope.push_back(static_cast<unsigned char>((msg_size >> 8) & 0xFF));

    // Вычисляем CRC сообщения msg
    std::array<unsigned char, HASH_SIZE> msg_crc = Crypt::calcCRC(msg);
    std::vector<unsigned char> vcrc;
    vcrc.insert(vcrc.end(), msg_crc.begin(), msg_crc.end());
    dbg_out_vec(":msg_crc", vcrc);

    // Записываем в envelope msg_crc, после msg_size
    envelope.insert(envelope.end(), msg_crc.begin(), msg_crc.end());

    // Вычисляем подпись сообщения msg_sign
    auto optSign = Crypt::sign(msg, private_key);
    if (!optSign) {
        throw std::runtime_error("Signing message failed");
    }
    std::array<unsigned char, 512> msg_sign = *optSign;

    // Записываем в envelope msg_sign, после msg_crc
    envelope.insert(envelope.end(), msg_sign.begin(), msg_sign.end());
    std::vector<unsigned char> vsign;
    vsign.insert(vsign.end(), msg_sign.begin(), msg_sign.end());
    dbg_out_vec(":msg_sign", vsign);

    // Записываем в envelope msg, после msg_sign
    envelope.insert(envelope.end(), msg.begin(), msg.end());
    std::vector<unsigned char> vmsg;
    vmsg.insert(vmsg.end(), msg.begin(), msg.end());
    // dbg_out_vec(":msg", vmsg);

    // dbg envelope
    dbg_out_vec(":envelope", envelope);

    /**
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

    // Разбиваем envelope на chunks
    std::vector<std::vector<unsigned char>> chunks = splitVec(envelope, CHUNK_SIZE);

    // Шифруем каждый chunk публичным ключом и записываем в enc_chunks
    std::vector<std::vector<unsigned char>> enc_chunks;
    for (const auto& chunk : chunks) {
        // dbg
        dbg_out_vec(":chunk", chunk);
        // ENCRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_enc_chunk =
            Crypt::Encrypt(chunk, public_key);
        if (!(opt_enc_chunk)) {
            std::cerr << "Error: Encryption Chunk Failed" << std::endl;
            throw "qwqewqewqe";
        }
        // SAVE ENCRYPT CHUNK
        std::vector<unsigned char> enc_chunk = *opt_enc_chunk;
        enc_chunks.push_back(enc_chunk);
        // dbg
        dbg_out_vec(":enc_chunk", enc_chunk);
    }

    // Формируем cipher для передачи по сети
    std::vector<unsigned char> cipher;
    cipher.clear();

    // Первым записываем размер envelope в chunks, младший байт первым
    uint16_t envelope_size = static_cast<uint16_t>(enc_chunks.size());
    dbg_out_scalar(":envelope_size", envelope_size, 2);
    cipher.push_back(static_cast<unsigned char>(envelope_size & 0xFF));
    cipher.push_back(static_cast<unsigned char>((envelope_size >> 8) & 0xFF));

    // Потом записываем все enc_chunks
    for (const auto& enc_chunk : enc_chunks) {
        cipher.insert(cipher.end(), enc_chunk.begin(), enc_chunk.end());
    }

    // dbg_out_vec(":cipher", cipher);
    return cipher;
}

/**
   +-------------------------+
   | envelope_size_in_chunks | 2 bytes
   +-------------------------+
   | enc_chunks              |
   |       ....              |
   +-------------------------+
*/

std::string decipher(
    EVP_PKEY* private_key, EVP_PKEY* public_key, std::vector<unsigned char> cipher)
{
    std::cout << "--------------------" <<std::endl;

    dbg_out_vec("|cipher", cipher);

    // Извлекаем размер envelope выраженный в количестве чанков
    uint16_t envelope_size =
        static_cast<uint16_t>(cipher[0]) | (static_cast<uint16_t>(cipher[1]) << 8);
    dbg_out_scalar("|envelope_size", envelope_size, 2);

    // Извлекаем enc_chunks из cipher
    std::vector<std::vector<unsigned char>> enc_chunks;
    auto it = cipher.begin() + 2;
    for (size_t i = 0; i < envelope_size; ++i) {
        if (std::distance(it, cipher.end()) <static_cast<ptrdiff_t>(ENC_CHUNK_SIZE)) {
            throw std::runtime_error("Not enough data to read chunk");
        }
        enc_chunks.emplace_back(it, it + ENC_CHUNK_SIZE);
        it += ENC_CHUNK_SIZE;
    }

    // дешифруем каждый enc_chunk приватным ключом и записываем в chunks
    std::vector<std::vector<unsigned char>> chunks;
    // size_t bytes_left = size_of_msg;
    for (std::vector<unsigned char> chunk : enc_chunks) {
        // dbg encrypted chunk
        // dbg_out_vec("|enc-chunk", chunk);
        // DECRYPT CHUNK
        std::optional<std::vector<unsigned char>> opt_dec_chunk =
            Crypt::Decrypt(chunk, private_key);
        if (!opt_dec_chunk) {
            std::cerr << "Error: Decryption Chunk Failed" << std::endl;
            throw "nytnyunygng";
        }
        // SAVE DECRYPTED CHUNK
        std::vector<unsigned char> dec_chunk = *opt_dec_chunk;
        // corrections
        // if (bytes_left < 255) {
        //     dec_chunk.resize(bytes_left);
        //     bytes_left = 0;
        // } else {
        //     bytes_left -= 255;
            dec_chunk.resize(CHUNK_SIZE);
        // }
        // PUSH RESULT
        chunks.push_back(dec_chunk);
        // dbg decrypted chunk
        dbg_out_vec("|dec-chunk", dec_chunk);
    }

    // Формируем envelope
    std::vector<unsigned char> envelope;
    for (const auto& enc_chunk : chunks) {
        envelope.insert(envelope.end(), enc_chunk.begin(), enc_chunk.end());
    }

    dbg_out_vec("|envelope", envelope);


    uint8_t random_len = static_cast<uint8_t>(envelope[0]);
    dbg_out_scalar("|random_len", random_len, 1);

    size_t offset = random_len + 1;

    uint16_t msg_size =
        static_cast<uint16_t>(envelope[offset]) |
        (static_cast<uint16_t>(envelope[offset+1]) << 8);
    offset += 2;
    dbg_out_scalar("|msg_size", msg_size, 2);

    std::vector<unsigned char> msg_crc;
    msg_crc.insert(
        msg_crc.end(), envelope.begin()+offset, envelope.begin()+offset+HASH_SIZE);
    offset += HASH_SIZE;
    std::vector<unsigned char> vcrc;
    vcrc.insert(vcrc.end(), msg_crc.begin(), msg_crc.end());
    dbg_out_vec("|msg_crc", vcrc);

    std::vector<unsigned char> msg_sign;
    msg_sign.insert(
        msg_sign.end(), envelope.begin()+offset, envelope.begin()+offset+512);
    offset += 512;
    std::vector<unsigned char> vsign;
    vsign.insert(vsign.end(), msg_sign.begin(), msg_sign.end());
    dbg_out_vec("|msg_sign", vsign);

    std::string result(envelope.begin()+offset, envelope.begin()+offset+msg_size);
    std::cout << "result: [" << result << "]" << std::endl;

    return result;
}

bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key, std::string msg)
{
    bool result = false;

    std::vector<unsigned char> cipher = encipher(private_key, public_key, msg);

    std::string new_msg = decipher(private_key, public_key, cipher);


    // MsgRec msgStruct2;

    // msgStruct2.envelope = msgStruct.envelope;

    // std::cout << "size1: " << msgStruct.envelope.size() << std::endl;
    // std::cout << "size2: " << msgStruct2.envelope.size() << std::endl;

    // // Извлекаем size_of_msg из envelope
    // if (msgStruct2.envelope.size() < 2) {
    //     throw std::runtime_error("Vector too small to extract uint16_t");
    // }
    // msgStruct2.size_of_msg =
    //     static_cast<uint16_t>(msgStruct2.envelope[0]) |
    //     (static_cast<uint16_t>(msgStruct2.envelope[1]) << 8);
    // std::cout << "cnt2: " << msgStruct2.size_of_msg << std::endl;

    // // Извлекаем зашифрованные chunks из envelope
    // std::vector<unsigned char> vec = msgStruct2.envelope;
    // size_t chunkSize = 512;
    // size_t numChunks = msgStruct2.size_of_msg / 255;
    // size_t remChunks = msgStruct2.size_of_msg % 255;
    // if (remChunks > 0) { numChunks++; }
    // std::vector<std::vector<unsigned char>> chunks;
    // // начинаем с третьего байта, т.к. первые два - uint16_t size
    // auto it = vec.begin() + 2;
    // for (size_t i = 0; i < numChunks; ++i) {
    //     if (std::distance(it, vec.end()) < static_cast<ptrdiff_t>(chunkSize)) {
    //         throw std::runtime_error("Not enough data to read chunk");
    //     }
    //     chunks.emplace_back(it, it + chunkSize);
    //     it += chunkSize;
    // }
    // msgStruct2.enc_chunks = chunks;

    // // dbgout
    // // for (const auto& chunk : msgStruct2.enc_chunks) {
    // //     std::vector<unsigned char> vch(chunk.begin(), chunk.end());
    // //     dbg_out_vec("\n\nENC_CH_2:", vch);
    // // }

    // // Расшифровываем каждый enc_chunk и сохраняем в chunks
    // msgStruct2.chunks.clear();
    // msgStruct2.chunks =
    //     dec(msgStruct2.size_of_msg, msgStruct2.enc_chunks, private_key);

    // // Объединяем все chunks в одно сообщение
    // msgStruct2.msg.clear();
    // std::vector<unsigned char> tmp = mergeVec(msgStruct2.chunks);
    // msgStruct2.msg.insert(msgStruct2.msg.end(), tmp.begin(), tmp.end());
    // std::cout << "\nmsg2: " << msgStruct2.msg << std::endl;


    // // Вычисляем и проверяем CRC сообщения
    // auto calculatedCrc = Crypt::calcCRC(msgStruct.msg);
    // if (calculatedCrc != msgStruct.crc_of_msg) {
    //     throw std::runtime_error("CRC check failed");
    // }

    // // Восстанавливаем size_of_msg
    // msgStruct.size_of_msg = static_cast<uint16_t>(msgStruct.msg.size());


    // // // Extract signature from reconstructed message
    // // std::vector<unsigned char> ext_sign(re_msg.begin(), re_msg.begin() + 512);
    // // std::string ext_msg = re_msg.substr(512);

    // // // CHECK SIGNATURE
    // // if (!Crypt::verify(ext_msg, *reinterpret_cast<std::array<unsigned char, 512>*>(
    // //                        ext_sign.data()), public_key))
    // // {
    // //     std::cerr << "Error: Message Signature Verification Failed" << std::endl;
    // //     return false;
    // // }
    // // std::cout << "Message Signature Verification Ok" << std::endl;


    // // // CHECK CRC
    // // if (!Crypt::verifyChecksum(ext_msg, ext_crc)) {
    // //     std::cerr << "Error: Checksum verification failed" << std::endl;
    // //     return false;
    // // }
    // // std::cout << "Message Checksum verificationn Ok" << std::endl;

    // // // Если все проверки прошли успешно и сообщения равны
    // // result = (message == ext_msg);
    // // if (false == result) {
    // //     std::cout << "\next_msg: " << ext_msg << std::endl;
    // // }
    // // std::cout << "ext_msg: " << ext_msg <<std::endl;


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
