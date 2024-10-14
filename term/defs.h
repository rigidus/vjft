#ifndef DEFS_H
#define DEFS_H

#ifndef DBG_MSG
  #define DBG_MSG 0
#endif

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define SIG_SIZE 512
#define HASH_SIZE 32
#define CHUNK_SIZE 255
#define ENC_CHUNK_SIZE 512
// Минимальная длина пакета:
// - 2 байта длины
// - Envelope (1536 байт) :
//   - 1 байт random_len
//   - 0 байт random
//   - 2 байта msg_size
//   - 32 байта msg_crc
//   - 512 байта msg_sign
//   - 1 байт msg
//   Всего 548 байт, это 3 незашифрованных чанка по 255 байт
//   После шифрования чанки будут по 512 байт, значит 1536 байт
// - 32 байта Sync marker
// = 1570 байт
#define MIN_PACK_SIZE 1570
// Максимальная длина пакета:
// - 32 чанка по 512 байт в Envelope = 16385
// - 2 байта длины
// - 32 байта Sync marker
// = 16418
#define MAX_MSG_SIZE 16418
#define MAX_PACK_SIZE 16384
#define READ_QUEUE_SIZE 32
#define SYNC_MARKER_SIZE 32

#define LOG_HEX(msg, value, size)                           \
    if (DBG_MSG > 0) {                                      \
        printf(":> %s :: %s(): ", __FILE__, __FUNCTION__);  \
        printf(" %s:\n", msg);                              \
        for (int i = 0; i < size; i++) {                    \
            printf("%02x ", *((uint8_t*)value + i));        \
        }                                                   \
        printf("\n");                                       \
    }


#define LOG_TXT(msg)                                \
    if (DBG_MSG > 0) {                              \
        fprintf(stderr, "Err: %s\n :>%s::%s", msg,  \
                __FILE__, __FUNCTION__);            \
    }

#endif // DEFS_H
