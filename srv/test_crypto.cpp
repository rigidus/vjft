// test_crypto.cpp
#include "test_crypto.hpp"
#include <openssl/err.h>

void dbg_out_vec(std::string dbgmsg, std::vector<unsigned char> vec) {
    std::cout << "\n" << dbgmsg << " [ ";
    for (const auto& byte : vec) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << "]" << std::endl; // Возвращаемся к десятичному формату
}

bool TestFullSequence(EVP_PKEY* private_key, EVP_PKEY* public_key, std::string message)
{
    bool result = false;

    // SIZE
    uint16_t size = message.size();

    // MSG_TO_CRC
    std::array<unsigned char, HASH_SIZE> crc = Crypt::calcCRC(message);
    // DBGOUT CRC
    std::vector<unsigned char> vcrc(crc.begin(), crc.end());
    dbg_out_vec("\n\nCRC:", vcrc);

    // SIGN MESSAGE
    std::optional<std::array<unsigned char, 512>> optSign =
        Crypt::sign(message, private_key);
    if (!optSign) {
        std::cerr << "Error: Signing Message Failed" << std::endl;
        return result;
    }
    std::array<unsigned char, 512> sign = *optSign;

    // Append signature to message
    std::string signed_message =
        std::string(sign.begin(), sign.end()) +
        message;

    // MSG_TO_CHUNKS
    std::vector<std::vector<unsigned char>> msg_chunks =
        Message::splitStrToVec255(signed_message);

    // TO_BUFFER
    // - crc :32
    std::vector<unsigned char> buffer;
    buffer.insert(buffer.end(), crc.begin(), crc.end());
    // - size :2
    unsigned char sizeBytes[sizeof(size)];
    std::memcpy(sizeBytes, &size, sizeof(size));
    buffer.insert(buffer.end(), sizeBytes, sizeBytes + sizeof(size));
    // // - signature chunks
    // buffer.insert(buffer.end(), signChunk1.begin(), signChunk1.end());
    // buffer.insert(buffer.end(), signChunk2.begin(), signChunk2.end());
    // - encrypted & sign for all chunks :512*n
    for (const auto& chunk : msg_chunks) {
        // ENCRYPT CHUNK
        std::optional<std::vector<unsigned char>> optCryptChunk =
            Crypt::Encrypt(chunk, public_key);
        if (!(optCryptChunk)) {
            std::cerr << "Error: Encryption Chunk Failed" << std::endl;
            return result;
        }
        // SAVE ENCRYPT CHUNK
        std::vector<unsigned char> cryptChunk = *optCryptChunk;
        buffer.insert(buffer.end(), cryptChunk.begin(), cryptChunk.end());
    }

    // --------------------------------------------------

    // DBGOUT BUFFER
    // dbg_out_vec("\n\nBuffer:", buffer);

    // --------------------------------------------------

    size_t offset = 0;

    // BUFFER_TO_CRC
    std::array<unsigned char, HASH_SIZE> ext_crc;
    std::memcpy(ext_crc.data(), buffer.data() + offset, HASH_SIZE);
    offset += HASH_SIZE;

    // BUFFER_TO_SIZE
    uint16_t extracted_size;
    std::memcpy(&extracted_size, buffer.data() + offset, sizeof(extracted_size));
    offset += sizeof(extracted_size);
    std::cout << "\nextracted_size: " << extracted_size << std::endl;
    uint16_t extracted_size_save = extracted_size;

    // FROM_BUFFER
    std::vector<unsigned char> reconstruct;
    while (offset < buffer.size()) {
        // EXTRACT CHUNK
        std::vector<unsigned char> encrypted_chunk(buffer.begin() + offset,
                                                   buffer.begin() + offset + 512);
        offset += 512;
        // DECRYPT CHUNK
        std::optional<std::string> decrypted_chunk =
            Crypt::Decrypt(encrypted_chunk, private_key);
        if (!decrypted_chunk) {
            std::cerr << "Error: Decryption Chunk Failed" << std::endl;
            return false;
        }
        // CHOP CHUNK
        std::string chop;
        if (extracted_size > 255) {
            chop = decrypted_chunk->substr(0, 255);
            extracted_size -= 255;
        } else {
            chop = decrypted_chunk->substr(0, extracted_size);
            extracted_size = 0;
        }
        std::cout << "\nchop size: " << chop.size() << std::endl;
        std::cout << "\nchop: " << chop << std::endl;
        // Добавление расшифрованного чанка в восстановленное сообщение
        reconstruct.insert(reconstruct.end(), chop.begin(), chop.end());
    }

    // VERIFY CRC
    if (ext_crc != crc) {
        std::cerr << "Error: CRC mismatch" << std::endl;
        return false;
    }

    // CHECK SIZE
    int re_size = reconstruct.size();
    if (re_size != extracted_size_save) {
        std::cerr << "Error: Size mismatch: " << re_size << std::endl;
        return false;
    } else {
        std::cout << "\nSize ok: " << re_size << std::endl;
    }

    // STRINGIFY SIZE
    std::string re_str(reconstruct.begin(), reconstruct.end());

    // CHUNKS_TO_RE_MSG
    std::string re_msg = Message::joinVec255ToStr(msg_chunks);

    // Extract signature from reconstructed message
    std::vector<unsigned char> ext_sign(re_msg.begin(), re_msg.begin() + 512);
    std::string ext_msg = re_msg.substr(512);

    // CHECK SIGNATURE
    if (!Crypt::verify(ext_msg, *reinterpret_cast<std::array<unsigned char, 512>*>(
                           ext_sign.data()), public_key))
    {
        std::cerr << "Error: Message Signature Verification Failed" << std::endl;
        return false;
    }

    // CHECK CRC
    if (!Crypt::verifyChecksum(ext_msg, ext_crc)) {
        std::cerr << "Error: Checksum verification failed" << std::endl;
        return false;
    }

    // Если все проверки прошли успешно и сообщения равны
    result = (message == ext_msg);
    if (false == result) { std::cout << "\next_msg: " << ext_msg << std::endl; }

    std::cout << "\nTestFullSequence End" << std::endl;

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
