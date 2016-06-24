#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAXSIZE 256

int get_port(char *);
void no_name(t_client);
void bad_format(t_client);
void print_to(t_client, char *);
int read_client_data(t_client, char **);
t_room make_room(char *, char *);
void print_to_room(t_room, t_client, char *);
char *get_status(t_room, char *);
void print_user_room(t_room, t_client);
void send_message(t_room, t_client, char *);
void del_last(char *s);
char *get_word(char *);
