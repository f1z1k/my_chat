#include "list.h"
#include "etc.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int load_clients(char *name_file)
{
	t_client client;
	FILE *file = fopen(name_file, "r");
	char *nick, *passwd;
	if (file == NULL) {
		perror("my_server:error load clients");
		return 0;
	}
	nick = (char *) malloc(MAXWORDLEN);
	passwd = (char *) malloc(MAXWORDLEN);
	while (fscanf(file, "%s %s\n", nick, passwd) != EOF) {
		client = (t_client) malloc(sizeof(*client));
		client->nick = nick;
		client->passwd = passwd;
		add_client(OFFLINE, client);
		nick = (char *) malloc(MAXWORDLEN);
		passwd = (char *) malloc(MAXWORDLEN);
	}
	free(nick); free(passwd);
	fclose(file);
	return 1;
}

int save_clients(char *name_file)
{
	t_client del, client = get_offline_client();
	FILE *file = fopen(name_file, "w");
	if (file == NULL) {
		perror("my_server:error save clients");
		return 0;
	}
	while (client) {
		fprintf(file, "%s %s\n", client->nick, client->passwd);
		del = client;
		client = client->next;
		destroy_client(del);
	}
	fclose(file);
	return 1;
}

char *get_str(FILE *file)
{
	char *str  = (char *) malloc(1000);
	int c, i = 0;
	while ((c =getc(file)) != '\n')
		str[i++] = c;
	str[i] = '\0';
	return str;
}

int load_rooms(char *name_file)
{
	t_room  room;
	FILE *file = fopen(name_file, "r");
	char *name, *admin, *str;
	if (file == NULL) {
		perror("my_server:error load rooms");
		return 0;
	}
	name = (char *) malloc(MAXWORDLEN);
	admin = (char *) malloc(MAXWORDLEN);
	while (fscanf(file, "%s %s\n", name, admin) != EOF) {
		char *nick;
		room = (t_room) malloc(sizeof(*room));
		room->name = name;
		room->admin = admin;
		room->topic = get_str(file);

		str = get_str(file);
		while (strlen(str) != 0) {
			nick = get_word(str);
			add_nick(MODER, room, nick);
		}
		str = get_str(file);
		while (strlen(str) != 0) {
			nick = get_word(str);
			add_nick(BAN, room, nick);
		}
		add_room(room);
		name = (char *) malloc(MAXWORDLEN);
		admin = (char *) malloc(MAXWORDLEN);

	}
	free(name); free(admin);
	fclose(file);
	return 1;
}

int save_rooms(char *name_file)
{
	t_room del = NULL, room = get_hall();
	t_nick nick;
	FILE *file = fopen(name_file, "w");
	char *topic;
	if (file == NULL) {
		perror("my_server:error save clients");
		return 0;
	}
	room = room->next;
	while (room) {
		fprintf(file, "%s %s\n", room->name, room->admin);
		topic = room->topic;
		fprintf(file, "%s\n", topic ? topic : "Без темы");

		for (nick = room->moder_list; nick; nick = nick->next)
			fprintf(file, "%s ", nick->nick);
		fprintf(file, "\n");
		for (nick = room->ban_list; nick; nick = nick->next)
			fprintf(file, "%s ", nick->nick);
		fprintf(file, "\n");

		del = room;
		room = room->next;
		destroy_room(del);
	}
	fclose(file);
	return 1;
}
