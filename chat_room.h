#ifndef CHAT_ROOM_H_INCLUDE
#define CHAT_ROOM_H_INCLUDE

#include <list>
#include <algorithm>
using namespace std;

#define C2S_LOGIN 1
#define C2S_LOGOUT 2
#define C2S_ONLINE_USER 3

#define MSG_LEN 1024

#define S2C_LOGIN_OK 1
#define S2C_ALREADY_LOGINED 2
#define S2C_SOMEONE_LOGIN 3
#define S2C_SOMEONE_LOGOUT 4
#define S2C_ONLINE_USER 5

#define C2C_CHAT 6

struct message
{
	int cmd;
	char body[MSG_LEN];
};

struct user_info
{
	char username[16];
	unsigned int ip;
	unsigned short port;
};

struct chat_msg
{
	char username[16];
	char msg[100];
};

//list<user_info> user_list;

#endif //CHAT_ROOM_H_INCLUDE
