#include "list.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#define MAXSIZE 256
/* получить числовое представление */
int get_port(char *s)
{
	int port = 0;
	if (s) {
		sscanf(s, "%d", &port);
		if (port > 0) return port;
	}
	do {
		printf("port\n");
		scanf("%d", &port);
	} while (port <= 0);
	return port;
}

/* ошибка */
void no_name(t_client client)
{
	char *msg = (char *) malloc(MAXSIZE);
	sprintf(msg, "### Авторизуйтесь(зарегистрируйтесь), чтобы выполнить команду\n");
	write(client->sock, msg, strlen(msg) + 1);
	free(msg);
	return;
}

/* ошибка */
void bad_format(t_client client)
{
	char *msg = (char *) malloc(MAXSIZE);
	sprintf(msg, "### Неверный формат команды. Для помощи введите /help\n");
	write(client->sock, msg, strlen(msg) + 1);
	free(msg);
	return;
}

/* написать клиенту */
void print_to(t_client client, char *msg)
{
	write(client->sock, msg, strlen(msg) + 1);
	return;
}

/* считать сообщение */
int read_client_data(t_client client, char **s)
{
	char msg[MAXSIZE + 1], buf[MAXSIZE + 1];
	int len, n;
	int fd = client->sock;
	if ((n = read(fd, buf, MAXSIZE + 1)) < 1) return n;
	if (n > MAXSIZE) {
		int a = MAXSIZE + 1;
		while (a == MAXSIZE + 1)
			a = read(fd, buf, MAXSIZE + 1);
		return n;
	}
	len = n;
	strncpy(msg, buf, n);
	msg[len] = '\0';
	while (msg[len - 1] != '\n' && msg[len - 1] != '\r') {
		n = read(fd, buf, MAXSIZE);
		len = len + n;
		if (len > MAXSIZE) {
			int a = MAXSIZE;
			while (a == MAXSIZE + 1)
				a = read(fd, buf, MAXSIZE);
			return len;
		}
		strncat(msg, buf, n);
		msg[len]  = '\0';
	}
	*s = (char *) malloc(len + 1);
	strcpy(*s, msg);
	return len;
}

t_room make_room(char *nick, char *name)
{
	t_room room = (t_room) malloc (sizeof (*room));
	room->name = name;
	room->topic = NULL;
	room->admin = (char *) malloc (strlen(nick) + 1);
	strcpy(room->admin, nick);
	room->ban_list = NULL;
	room->moder_list = NULL;
	room->client_list = NULL;
	add_nick(MODER, room, nick);
	return room;
}

void print_to_room(t_room room, t_client hoster, char *s)
{
	t_client client;
	for (client = room->client_list; client; client = client->next) {
		if (client != hoster && client->nick)
			write(client->sock, s, strlen(s));
	}
	return;
}

char *get_status(t_room room, char *nick)
{
	char *status = (char *) malloc (20);
	if (room->admin && strcmp(nick, room->admin) == 0)
		strcpy(status, "admin");
	else if (find_nick(MODER, room, nick))
		strcpy(status, "moder");
	else
		strcpy(status, "user");
	return status;
}

void print_user_room(t_room room, t_client questioner)
{
	char *nick;
	t_client client;
	char *msg = (char *) malloc (MAXSIZE);
	sprintf(msg, "### В комнате %s находятся %d клиент(а/ов):\n", room->name, 
				get_num_clients(room)
	);
	print_to(questioner, msg);
	for (client = room->client_list; client; client = client->next) {
		nick = client->nick;
		if (nick == NULL) continue;
		sprintf(msg, "### %s %s\n", nick, get_status(room, nick));
		print_to(questioner, msg);
	}
	free(msg);
	return;
}

void send_message(t_room room, t_client client, char *s)
{
	char *msg = (char *) malloc (MAXSIZE);
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	if (room != get_hall()) {
		sprintf(msg, "<%s>: %s\n", client->nick, s);
		print_to_room(room, client, msg);
		free(msg);
	} else
		print_to(client, "### Эта прихожая не для общения\n");
	return;
}

/*
	сообщение может заканчиваться на разный комбинации
	символов
*/
void del_last(char *s)
{
	int n = 0;
	while (s[n] != '\r' && s[n] != '\n') n++;
	s[n] = '\0';
	return;
}

/* выделить слово из сообщения */
char *get_word(char *data)
{
	int n = 0, word_len = 0;
	char c;
	char *word;
	int i;
	while ((c = data[n]) != 0 && isspace(c))
		n++;
	while ((c = data[n + word_len]) != 0 && !isspace(c)) word_len++;
	word = (char *) malloc (word_len + 1);
	strncpy(word, data, word_len);
	word[word_len] = '\0';
	n +=  word_len;
	while ((c = data[n]) != 0 && isspace(c))
		n++;
	for (i = n; data[i] && data[i] != '\r' && data[i] != '\n'; i++)
		data[i - n] = data[i];
	data[i - n] = '\0';
	return word;
}

