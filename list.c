#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define BAN 1
/* список клиентов */
typedef struct client {
	char * nick;
	char * passwd;
	int sock;
	struct client * next;
} *t_client;

/* список операторов */
/* или бан лист по никам */
typedef struct nick {
	char *nick;
	struct nick *next;
} *t_nick;

/* список комнат */
typedef struct room {
	char *name;
	char *topic;
	char *admin;
	t_nick moder_list;
	t_nick ban_list;
	t_client client_list;
	struct room *next;
} *t_room;

t_client offline_list = NULL; /* зарег оффлайн клиенты */
t_room room_list = NULL;

t_room get_hall()
{
	return room_list; /* прихожая, c неё начинаем обходить */
}

void add_room(t_room room)
{
	room->next = room_list->next;
	room_list->next = room;
	return;
}

/* методы для работы с списками */
void rm_all_nicks(t_room room);
void rm_room(t_room room)
{
	t_room prev = room_list;
	if (room == room_list) {
		room_list = room->next;
		free(room);
		return;
	}

	while (prev->next != room)
		prev = prev->next;
	prev->next = room->next;

	free(room->name);
	if (room->topic) free(room->topic);
	free(room->admin);
	rm_all_nicks(room);
	free(room);
	return;
}

void add_client(t_room room, t_client client)
{
	if (room == NULL) {
		client->next = offline_list;
		offline_list = client;
	} else {
		client->next = room->client_list;
		room->client_list = client;
	}
	return;
}

void rm_client(t_room room, t_client client)
{
	t_client client_list = (room) ? room->client_list
								: offline_list;
	t_client prev = client_list;
	if (client != client_list) {
		while (prev->next != client)
			prev = prev->next;
		prev->next = client->next;
	} else
		client_list = client->next;
	if (room)
		room->client_list = client_list;
	else
		offline_list = client_list;
	return;
}

/* поиск оффлайн клиента */
t_client find_offline_client(char *nick)
{
	t_client client;
	for (client = offline_list; client; client = client->next) {
		if (strcmp(client->nick, nick) == 0)
			return client;
	}
	return NULL;
}

void destroy_client(t_client client)
{
	if (client->nick) free(client->nick);
	if (client->passwd) free(client->passwd);
	free(client);
	return;
}

int get_num_clients(t_room room)
{
	int num = 0;
	t_client client;
	for (client = room->client_list; client; client = client->next) {
		if (client->nick)
			num++;
	}
	return num;
}
void add_nick(int status, t_room room, char *nick)
{
	t_nick list = status == BAN ? room->ban_list 
								: room->moder_list;
	t_nick tmp = (t_nick) malloc (sizeof (*tmp));
	tmp->nick = (char *) malloc(strlen(nick) + 1);
	strcpy(tmp->nick, nick);
	tmp->next = list;
	list = tmp;	
	if (status == BAN)
		room->ban_list = list;
	else
		room->moder_list = list;
}

void rm_nick(int status, t_room room, char *nick)
{
	t_nick list = status == BAN ? room->ban_list 
								: room->moder_list;
	t_nick prev, del = list;
	if (del == NULL) return;
	if (strcmp(del->nick, nick) == 0) {
		list = list->next;
		free(del);
		if (status == BAN)
			room->ban_list = list;
		else
			room->moder_list = list;
		return;
	}
	while (del != NULL) {
		if (strcmp(del->nick, nick) == 0) {
			prev->next = del->next;
			free(del);
			return;
		}
		prev = del;
		del = del->next;
	}	
}

void rm_all_nicks(t_room room)
{
	t_nick del, nick = room->ban_list;
	while (nick) {
		del = nick;
		nick = nick->next;
		free(del->nick);
		free(del);
	}
	nick = room->moder_list;
	while (nick) {
		del = nick;
		nick = nick->next;
		free(del->nick);
		free(del);
	}
	return;
}
int find_nick(int status,t_room room, char *nick)
{
	t_nick list = status == BAN ? room->ban_list 
								: room->moder_list;
	while (list) {
		if (strcmp(list->nick, nick) == 0) 
			return 1;
		list = list->next;
	}
	return 0;
}

		
t_room find_room(char *name)
{
	t_room room;
	for (room = room_list; room; room = room->next) {
		if (strcmp(room->name, name) == 0)
			return room;
	}
	return NULL;
}

t_client find_client(t_room room, char *nick)
{
	t_client client_list, client;
	if (room) {
		client_list = room->client_list;
		for (client = client_list; client; client = client->next) {
			if (client->nick && strcmp(client->nick, nick) == 0)
				return client;
		}
		return NULL;
	}

	for (room = room_list; room; room = room->next) {
		client_list = room->client_list;
		for (client = client_list; client; client = client->next) {
			if (client->nick && strcmp(client->nick, nick) == 0)
				return client;
		}
	}
	return NULL;
}

t_client get_offline_client()
{
	return offline_list;
}

void make_hall()
{
	room_list = malloc(sizeof(*room_list));
	room_list->name = "hall";
	room_list->topic = "здесь нет обсуждений";
	room_list->admin = NULL;
	room_list->moder_list = NULL;
	room_list->ban_list = NULL;
	room_list->next = NULL;
	return;
}

void destroy_room(t_room room)
{
	free(room->name);
	free(room->admin);
	rm_all_nicks(room);
	free(room);
	return;
}
