#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <errno.h>
#include <cstdio>
#include <cstring>
#include <signal.h>
#include <sys/select.h>
#include <list>
#include "chat_room.h"

list<struct user_info> client_list;

void do_login(struct message &msg, int sock, struct sockaddr_in *cliaddr);
void do_logout(struct message &msg, int sock, struct sockaddr_in *cliaddr);
void do_sendlist(int sock, struct sockaddr_in *cliaddr);

void chat_srv(int sock)
{
	struct sockaddr_in cliaddr;
	socklen_t clilen;
	int n;
	struct message msg;
	while(1)
	{
		memset(&msg, 0, sizeof(msg));
		clilen = sizeof(cliaddr);
		n = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&cliaddr, &clilen);
		if(n < 0)
		{
			if(errno == EINTR)
				continue;
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}
		int cmd = ntohl(msg.cmd);
		//printf("This is char_srv...\n");
		switch(cmd)
		{
		case C2S_LOGIN:
			do_login(msg, sock, &cliaddr);
			break;
		case C2S_LOGOUT:
			do_logout(msg, sock, &cliaddr);
			break;
		case C2S_ONLINE_USER:
			do_sendlist(sock, &cliaddr);
			break;
		default:
			break;
		}
	}
}

void do_login(struct message &msg, int sock, struct sockaddr_in *cliaddr)
{
	struct user_info user;
	strcpy(user.username, msg.body);
	user.ip = cliaddr->sin_addr.s_addr;
	user.port = cliaddr->sin_port;

	auto it = client_list.begin();
	for(; it!=client_list.end(); ++it)
	{
		if(strcmp(it->username, msg.body) == 0)
		{
			break;
		}
	}

	if(it == client_list.end())
	{
		printf("has a user login : %s <-> %s:%d\n", msg.body, inet_ntoa(cliaddr->sin_addr), cliaddr->sin_port);
		client_list.push_back(user);

		struct message reply_msg;
		memset(&reply_msg, 0, sizeof(msg));
		reply_msg.cmd = htonl(S2C_LOGIN_OK);
		sendto(sock, &reply_msg, sizeof(msg), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));

		int count = htonl((int)client_list.size());
		sendto(sock, &count, sizeof(int), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));

		printf("sending user list information to: %s <-> %s:%d\n", msg.body, inet_ntoa(cliaddr->sin_addr), cliaddr->sin_port);

		for(it = client_list.begin(); it!=client_list.end(); ++it)
		{
			sendto(sock, &*it, sizeof(list<user_info>), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));
		}

		for(it = client_list.begin(); it != client_list.end(); ++it)
		{
			if(strcmp(it->username, msg.body) == 0)
				continue;
			struct sockaddr_in peeraddr;
			memset(&peeraddr, 0, sizeof(peeraddr));
			peeraddr.sin_family = AF_INET;
			peeraddr.sin_port = it->port;
			peeraddr.sin_addr.s_addr = it->ip;

			msg.cmd = htonl(S2C_SOMEONE_LOGIN);
			memcpy(msg.body, &user, sizeof(user));
			if(sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&peeraddr, sizeof(peeraddr)) < 0)
			{
				perror("sendto");
				exit(EXIT_FAILURE);
			}

		}
	}
	else
	{
		printf("user %s has already logined\n", msg.body);
		struct message reply_msg;
		memset(&reply_msg, 0, sizeof(reply_msg));
		reply_msg.cmd = htonl(S2C_ALREADY_LOGINED);
		sendto(sock, &reply_msg, sizeof(reply_msg), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));
	}

}

void do_logout(struct message &msg, int sock, struct sockaddr_in *cliaddr)
{
	printf("has a user logout : %s <-> %s:%d\n", msg.body, inet_ntoa(cliaddr->sin_addr), htons(cliaddr->sin_port));

	auto it = client_list.begin();
	for(; it != client_list.end(); ++it)
	{
		if(strcmp(it->username, msg.body) == 0)
			break;
	}

	if(it != client_list.end())
		client_list.erase(it);

	for(it = client_list.begin(); it != client_list.end(); ++it)
	{
		if(strcmp(it->username, msg.body) == 0)
			continue;
		
		struct sockaddr_in peeraddr;
		memset(&peeraddr, 0, sizeof(peeraddr));
		peeraddr.sin_family = AF_INET;	
		peeraddr.sin_port = it->port;	
		peeraddr.sin_addr.s_addr = it->ip;

		msg.cmd = htonl(S2C_SOMEONE_LOGOUT);
		
		if(sendto(sock, &msg, sizeof(msg), 0, (struct sockaddr*)&peeraddr, sizeof(peeraddr)) < 0)
		{
			perror("sendto");
			exit(EXIT_FAILURE);
		}	
	}
}

void do_sendlist(int sock, struct sockaddr_in *cliaddr)
{
	struct message msg;
	msg.cmd = htonl(S2C_ONLINE_USER);
	sendto(sock, (const char*)&msg, sizeof(msg), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));

	int count = htonl((int)client_list.size());
	sendto(sock, (const char*)&count, sizeof(int), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));
	for(auto it = client_list.begin(); it != client_list.end(); ++it)
	{
		sendto(sock, &*it, sizeof(struct user_info), 0, (struct sockaddr*)cliaddr, sizeof(*cliaddr));
	}	
}

int main()
{
	int sock;
	if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}
	chat_srv(sock);
	return 0;
}
