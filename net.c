#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "cmd.h"
fd_set readfds;
int listen_sock;
void make_ready_sock_set()
{
	int max_d = listen_sock;
	t_room room;
	t_client client;
	int fd;
	FD_ZERO(&readfds);
	FD_SET(listen_sock, &readfds);

	for (room = get_hall(); room; room = room->next) {
		for (client = room->client_list; client; client = client->next) {
			fd = client->sock;
			FD_SET(fd, &readfds);
			if(fd > max_d)
				max_d = fd;
		}
	}
	if (select(max_d+1, &readfds, NULL, NULL, NULL) < 1) {
		perror("my_chat");
		stop_server(0);
	}
	return;
}

int creat_listen_sock(int port)
{	
	int ls = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	int opt = 1;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (0 != bind(ls, (struct sockaddr *) &addr, sizeof(addr))) {
		fprintf(stderr, "port: %d: %s\n", port, strerror(errno));
		return 0;
	}
	if (-1 == listen(ls, 5)) {
		perror("my_chat: creat listen sock");
		return 0;
	}
	listen_sock = ls;
	return 1;
}

int is_new_client()
{
	return FD_ISSET(listen_sock, &readfds);
}

int is_knock(int a)
{
	return FD_ISSET(a, &readfds);
}
void rm_knock(int a)
{
	FD_CLR(a, &readfds);
	return;
}

int get_listen_sock()
{
	return listen_sock;
}
void shutdown_server()
{
	shutdown(listen_sock, 2);
	close(listen_sock);
	return;
}
