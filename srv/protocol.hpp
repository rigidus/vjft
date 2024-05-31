// protocol.hpp

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

enum : unsigned
{
	// MAX_IP_PACK_SIZE = 512,
	// MAX_NICKNAME = 16,
	PADDING = 24
};


const size_t MAX_ROOM_ID = 32;
const size_t MAX_IP_PACK_SIZE = 512;
const size_t MAX_NICKNAME = 64;

struct ChatMessage {
    char room_id[MAX_ROOM_ID];
    char nickname[MAX_NICKNAME];
    char message[MAX_IP_PACK_SIZE - MAX_ROOM_ID - MAX_NICKNAME];
};

#endif /* PROTOCOL_HPP_ */
