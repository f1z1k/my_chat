#include <stdlib.h>
#define MODER 0
#define BAN 1
#define ALL_ROOM 0
#define OFFLINE 0
#define MAXWORDLEN 30
typedef struct client {
	char * nick;
	char * passwd;
	int sock;
	struct client * next;
} *t_client;

typedef struct nick {
	char *nick;
	struct nick *next;
} *t_nick;

typedef struct room {
	char *name;
	char *topic;
	char *admin;
	t_nick moder_list;
	t_nick ban_list;
	t_client client_list;
	struct room *next;
} *t_room;

void make_hall();
void add_room(t_room);
void rm_room(t_room);
void destroy_room(t_room);
t_room find_room(char *);
t_room get_hall(void);

void add_client(t_room, t_client);
void rm_client(t_room, t_client);
void destroy_client(t_client);
t_client find_client(t_room, char *);
t_client find_offline_client(char *);
t_client get_offline_client(void);
int get_num_clients(t_room);

void add_nick(int, t_room, char *);
void rm_nick(int, t_room, char *);
int find_nick(int, t_room, char *);
void rm_all_nicks(t_room);
