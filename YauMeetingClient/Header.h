#pragma once
#include<ctype.h>
#include<string>

struct TypeHeader {
	uint8_t type;
};

//type 1
struct LoginHeader {
	uint8_t username_len;
	uint8_t password_len;
	uint8_t type;
	uint8_t room_id;
	uint8_t roompwd_len;
};

//type 2
struct LoginResponseHeader {
	uint16_t result;
	uint8_t room_id;
};

//type 3
struct DataHeader {
	uint8_t usernamelen;
	uint8_t datatype;
	uint64_t timestamp;
	uint32_t partid;
	uint32_t totalparts;
	uint32_t data_len;
};

//type 4
struct ControlHeader {
	uint32_t data_len;
};

//type 5
struct LogoffHeader {
	uint8_t username_len;
};