// test_crypto.cpp
#include "test_crypto.hpp"
#include <openssl/err.h>

struct MsgRec {
    std::array<unsigned char, HASH_SIZE>        crc_of_msg;     // 32
    uint16_t                                    size_of_msg;    // 2
    std::array<unsigned char, 512>              sign_of_msg;    // 512
    std::string                                 msg;
    std::vector<std::vector<unsigned char>>     chunks;
    std::vector<std::vector<unsigned char>>     enc_chunks;
    std::vector<unsigned char>                  envelope;
};

void dbg_out_vec(std::string dbgmsg, std::vector<unsigned char> vec) {
    std::cout << dbgmsg << " ";
    for (const auto& byte : vec) {
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

std::vector<std::vector<unsigned char>> enc(
    std::vector<std::vector<unsigned char>> chunks,
    EVP_PKEY* public_key)
{
    std::vector<std::vector<unsigned char>> result;
    for (const auto& chunk : chunks) {
        // ENCRYPT CHUNK
        std::optional<std::vector<unsigned char>> optCryptChunk =
            Crypt::Encrypt(chunk, public_key);
        if (!(optCryptChunk)) {
            std::cerr << "Error: Encryption Chunk Failed" << std::endl;
            return result;
        }
        // SAVE ENCRYPT CHUNK
        std::vector<unsigned char> cryptChunk = *optCryptChunk;
        result.push_back(cryptChunk);
    }

    return result;
}

std::vector<std::vector<unsigned char>> dec(
    size_t size_of_msg,
    const std::vector<std::vector<unsigned char>> enc_chunks,
    EVP_PKEY* private_key)
{
    std::vector<std::vector<unsigned char>> result;
    size_t bytes_left = size_of_msg;
    for (std::vector<unsigned char> chunk : enc_chunks) {
        // dbg encrypted chunk
        // std::string ed(chunk.begin(), chunk.end());
        // std::cout << "\nencrypted_chunk: " << ed << std::endl;
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
        if (bytes_left < 255) {
            dec_chunk.resize(bytes_left);
            bytes_left = 0;
        } else {
            bytes_left -= 255;
            dec_chunk.resize(255);
        }
        // dbg decrypted chunk
        std::string de(dec_chunk.begin(), dec_chunk.end());
        std::cout << "\nde: " << bytes_left << ":|:" << de << std::endl;
        // PUSH RESULT
        result.push_back(dec_chunk);
    }
    return result;
}


bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key, std::string msg)
{
    bool result = false;

    MsgRec msgStruct;

    // Записываем msg в структуру
    msgStruct.msg = msg;

    // Вычисляем и записываем размер сообщения
    msgStruct.size_of_msg = static_cast<uint16_t>(msg.size());

    // Вычисляем и записываем CRC сообщения
    msgStruct.crc_of_msg = Crypt::calcCRC(msg);

    // Вычисляем и записываем подпись сообщения
    auto optSign = Crypt::sign(msg, private_key);
    if (!optSign) {
        throw std::runtime_error("Signing message failed");
    }
    msgStruct.sign_of_msg = *optSign;

    // Преобразуем msg в std::vector<unsigned char>
    std::vector<unsigned char> msgVec(msg.begin(), msg.end());

    // Разбиваем сообщение на chunks
    msgStruct.chunks = splitVec(msgVec, 255);

    // Шифруем каждый chunk и записываем в enc_chunks
    msgStruct.enc_chunks = enc(msgStruct.chunks, public_key);

    // Формируем envelope
    msgStruct.envelope.clear();
    // size_of_msg
    msgStruct.envelope.push_back(
        static_cast<unsigned char>(msgStruct.size_of_msg & 0xFF));
    msgStruct.envelope.push_back(
        static_cast<unsigned char>((msgStruct.size_of_msg >> 8) & 0xFF));
    // enc_chunks
    for (const auto& enc_chunk : msgStruct.enc_chunks) {
        msgStruct.envelope.insert(
            msgStruct.envelope.end(), enc_chunk.begin(), enc_chunk.end());
    }


    MsgRec msgStruct2;

    msgStruct2.envelope = msgStruct.envelope;

    std::cout << "size1: " << msgStruct.envelope.size() << std::endl;
    std::cout << "size2: " << msgStruct2.envelope.size() << std::endl;

    // Извлекаем size_of_msg из envelope
    if (msgStruct2.envelope.size() < 2) {
        throw std::runtime_error("Vector too small to extract uint16_t");
    }
    msgStruct2.size_of_msg =
        static_cast<uint16_t>(msgStruct2.envelope[0]) |
        (static_cast<uint16_t>(msgStruct2.envelope[1]) << 8);
    std::cout << "cnt2: " << msgStruct2.size_of_msg << std::endl;

    // Извлекаем зашифрованные chunks из envelope
    std::vector<unsigned char> vec = msgStruct2.envelope;
    size_t chunkSize = 512;
    size_t numChunks = msgStruct2.size_of_msg / 255;
    size_t remChunks = msgStruct2.size_of_msg % 255;
    if (remChunks > 0) { numChunks++; }
    std::vector<std::vector<unsigned char>> chunks;
    // начинаем с третьего байта, т.к. первые два - uint16_t size
    auto it = vec.begin() + 2;
    for (size_t i = 0; i < numChunks; ++i) {
        if (std::distance(it, vec.end()) < static_cast<ptrdiff_t>(chunkSize)) {
            throw std::runtime_error("Not enough data to read chunk");
        }
        chunks.emplace_back(it, it + chunkSize);
        it += chunkSize;
    }
    msgStruct2.enc_chunks = chunks;

    // dbgout
    // for (const auto& chunk : msgStruct2.enc_chunks) {
    //     std::vector<unsigned char> vch(chunk.begin(), chunk.end());
    //     dbg_out_vec("\n\nENC_CH_2:", vch);
    // }

    // Расшифровываем каждый enc_chunk и сохраняем в chunks
    msgStruct2.chunks.clear();
    msgStruct2.chunks =
        dec(msgStruct2.size_of_msg, msgStruct2.enc_chunks, private_key);

    // Объединяем все chunks в одно сообщение
    msgStruct2.msg.clear();
    std::vector<unsigned char> tmp = mergeVec(msgStruct2.chunks);
    msgStruct2.msg.insert(msgStruct2.msg.end(), tmp.begin(), tmp.end());
    std::cout << "\nmsg2: " << msgStruct2.msg << std::endl;


    // Вычисляем и проверяем CRC сообщения
    auto calculatedCrc = Crypt::calcCRC(msgStruct.msg);
    if (calculatedCrc != msgStruct.crc_of_msg) {
        throw std::runtime_error("CRC check failed");
    }

    // Восстанавливаем size_of_msg
    msgStruct.size_of_msg = static_cast<uint16_t>(msgStruct.msg.size());


    // // Extract signature from reconstructed message
    // std::vector<unsigned char> ext_sign(re_msg.begin(), re_msg.begin() + 512);
    // std::string ext_msg = re_msg.substr(512);

    // // CHECK SIGNATURE
    // if (!Crypt::verify(ext_msg, *reinterpret_cast<std::array<unsigned char, 512>*>(
    //                        ext_sign.data()), public_key))
    // {
    //     std::cerr << "Error: Message Signature Verification Failed" << std::endl;
    //     return false;
    // }
    // std::cout << "Message Signature Verification Ok" << std::endl;


    // // CHECK CRC
    // if (!Crypt::verifyChecksum(ext_msg, ext_crc)) {
    //     std::cerr << "Error: Checksum verification failed" << std::endl;
    //     return false;
    // }
    // std::cout << "Message Checksum verificationn Ok" << std::endl;

    // // Если все проверки прошли успешно и сообщения равны
    // result = (message == ext_msg);
    // if (false == result) {
    //     std::cout << "\next_msg: " << ext_msg << std::endl;
    // }
    // std::cout << "ext_msg: " << ext_msg <<std::endl;


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
