#include "net.h"
#include "file.h"
#include "list.h"
#include "etc.h"
#include <unistd.h>
#include <sys/socket.h>
#define PATH  "/dev/null"

char *msg;
/* читать из stdin */
char *get_input_str()
{
	char buf[MAXSIZE + 1], *str;
	int buf_len = read(0, buf, MAXSIZE + 1), len;
	char is_new_line = buf_len > 0 && buf[buf_len - 1] == '\n';
	if (is_new_line)
		buf[--buf_len] = '\0';
	if (buf_len < 1)
		return NULL;
	len = buf_len + 1;
	str = (char *) malloc(len);
	strncpy(str, buf, buf_len);
	str[len - 1] = '\0';
	while (buf_len > MAXSIZE && !is_new_line) {
		buf_len = read(0, buf, MAXSIZE);
		is_new_line = buf_len > 0 && buf[buf_len - 1] == '\n';
		if (is_new_line)
			buf[--buf_len] = '\0';
		if (buf_len >= 1) {
			str = realloc(str, len += buf_len);
			strncat(str, buf, buf_len);
			str[len - 1] = '\0';
		} else if (buf_len == -1) {
			free(str);
			return NULL;
		}
	}
	return str;	
}

/* возвращает указтель на /dev/null */
char *default_path()
{
	char *s = (char *) malloc(strlen(PATH) + 1);
	strcpy(s, PATH);
	return s;
}

void stop_server(int s);
void start_server(char *port_str)
{
	char *client_file, *room_file;
	make_hall();
	/* считывание клиентов из файла */
	do {
		printf("client's file\n");
		if (!(client_file = get_input_str()))
			client_file = default_path();
	} while (!load_clients(client_file));
	printf("%s: load clients: success\n", client_file);
	/* считывание комнат из файла */
	do {
		printf("room's file\n");
		if (!(room_file = get_input_str()))
			room_file = default_path();
	} while (!load_rooms(room_file));
	printf("%s: load rooms: success\n", room_file);
	free(client_file); free(room_file);
	msg = (char *) malloc(MAXSIZE);

	while (!creat_listen_sock(get_port(port_str)));
	printf("### SERVER STARTED\n");
	return;
}

void disconnect_all_clients();
void stop_server(int s)
{
	char *client_file, *room_file;
	free(msg);
	disconnect_all_clients();
	printf("\n");
	/* сохранение клиентов в файле */
	do {	
		printf("client's file\n");
		if (!(client_file = get_input_str()))
			client_file = default_path();
	} while (!save_clients(client_file));
	printf("%s: save clients: success\n", client_file);

	/* сохранение комнат в файле */
	do {
		printf("room's file\n");
		if (!(room_file = get_input_str()))
			room_file = default_path();
	} while (!save_rooms(room_file));
	printf("%s: save rooms: success\n", room_file);

	free(client_file); free(room_file);
	shutdown_server();
	printf("### SERVER FINISHED\n");
	_exit(0);
}

void process_client(t_room room, t_client client);
void process_clients()
{
	t_room room;
	t_client client;
	for (room = get_hall(); room; room = room->next) {
		for (client = room->client_list; client; client = client->next) {
			if (is_knock(client->sock)) { /* пользователь даёт команду */
				process_client(room, client); /* обработать(выполнить или ошибка) */
				rm_knock(client->sock); /* удалить приказ из списка */
			}
		}
	}
	return;
}

void run_cmd(t_room room, t_client client, char *s);
void process_client(t_room room, t_client client)
{
	char *data;
	int n = read_client_data(client, &data);
	if (n < 1) {
		if (client->nick) {
			t_room tmp;
			sprintf(msg, "### У клиента <%s> проблемы со связью.Отключение\n", client->nick);
			for (tmp = get_hall();tmp; tmp = tmp->next) 
				print_to_room(tmp, client, msg);
		}
		close(client->sock);
		rm_client(room, client);
		add_client(OFFLINE, client);
	} else if (n > MAXSIZE) {
		print_to (client, "### Слишком длинная команда\n");
	} else {
		/* выполнить команду */
		run_cmd(room, client, data);
		free(data);
	}
	print_to(client, "> ");
	return;
}

void disconnect_all_clients()
{
	t_room room;
	t_client del, client;
	for (room = get_hall(); room; room = room->next) {
		client = room->client_list;
		while (client) {
			print_to(client, "### Сервер завершает свою работу. Удачи\n");
			close(client->sock);
			rm_client(room, client);
			del = client;
			client = client->next;
			if (del->nick == NULL)
				destroy_client(del);
			else
				add_client(OFFLINE, del);
		}
	}
	return;
}

/* названия методов совпадают с именами команд */
void reg(t_client client, char *s);
void login(t_client client, char *s); 
void help(t_client client);

void nick(t_client client, char *s);
void passwd(t_client client, char *s);
void whisper(t_client client, char *s);
void users(t_client client, char *s);
void enter_room(t_room old_room, t_client client, char *s);
void rooms(t_client client);
void users(t_client client, char *s);
void leave (t_room room, t_client client, char *s);

void rmroom(t_room room, t_client admin, char *s);
void op(t_room room, t_client admin, char *s);
void deop(t_room room, t_client admin, char *s);

void ban(t_room room, t_client moder, char *s);
void unban(t_room room, t_client moder, char *s);
void kick(t_room room, t_client moder, char *s);
void topic(t_room room, t_client moder, char *s);

void run_cmd(t_room room, t_client client, char *s)
{
	del_last(s);
	if (s[0] != '/') {
		send_message(room, client, s);
	} else {
		char *cmd = get_word(s);
		if (strcmp(cmd,"/reg") == 0)
			reg(client, s);
		else if (strcmp(cmd, "/login") == 0)
			login(client, s);
		else if (strcmp(cmd, "/help") == 0)
			help(client);
		/* для авторизовавшихся пользователей */
		else if (strcmp(cmd, "/nick") == 0)
			nick(client, s);
		else if (strcmp(cmd, "/passwd") == 0)
			passwd(client, s);
		else if (strcmp(cmd, "/whisper") == 0)
			whisper(client, s);
		else if (strcmp(cmd, "/room") == 0)
			enter_room(room, client, s);
		else if (strcmp(cmd, "/rooms") == 0)
			rooms(client);
		else if (strcmp(cmd, "/users") == 0)
			users(client, s);
		else if (strcmp(cmd, "/leave") == 0)
			leave(room, client, s);
		/*для администраторов */
		else if (strcmp(cmd, "/rmroom") == 0)
			rmroom(room, client, s);
		else if (strcmp(cmd, "/op") == 0)
			op(room, client, s);
		else if (strcmp(cmd, "/deop") == 0)
			deop(room, client, s);
		/* для модераторов */
		else if (strcmp(cmd, "/ban") == 0)
			ban(room, client, s);
		else if (strcmp(cmd, "/unban") == 0)
			unban(room, client, s);
		else if (strcmp(cmd, "/kick") == 0)
			kick(room, client, s);
		else if (strcmp(cmd, "/topic") == 0)
			topic(room, client, s);
		else if (cmd[0] == '/') {
			sprintf(msg, "### Неверная команда: %s\n", cmd);
			print_to(client, msg);
		}
	}
	return;
}

void nick(t_client client, char *s)
{
	char *nick, *old_nick;
	t_room room;
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	if (strlen(nick = get_word(s)) == 0) {
		bad_format(client);
		return;
	}
	old_nick = client->nick;
	client->nick = nick;
	sprintf(msg, "### Ваш новый ник: %s\n", client->nick);
	print_to(client, msg);
	sprintf(msg, "*** Клиент %s изменил ник на %s\n", old_nick, client->nick);
	for (room = get_hall(); room; room = room->next)
		print_to_room(room, client, msg);
	free(old_nick);
	return;
}

void passwd(t_client client, char *s)
{
	char *passwd;
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	if (strlen(passwd = get_word(s)) == 0) {
		bad_format(client);
		return;
	}
	free(client->passwd);
	client->passwd = passwd;
	print_to(client, "### Вы изменили пароль\n");
	return;
}

void reg(t_client client, char *s)
{
	t_room room;
	char *nick = get_word(s);
	char *passwd = get_word(s);
	if (strlen(nick) == 0 || strlen(passwd) == 0 || strlen(nick) > 30) {
		bad_format(client);
		return;
	}
	if (find_client(NULL, nick) || find_offline_client(nick)) {
		print_to(client, "### Ник уже занят\n");
		return;
	}

	client->nick = nick;
	client->passwd = passwd;
	sprintf(msg, "*** Клиент %s зарегистрировался\n", nick);
	for (room = get_hall(); room; room = room->next)
		print_to_room(room, client, msg);
	sprintf(msg, "### Вы успешно зарегистрировались!!! Ваш ник %s\n", nick);
	print_to(client, msg);
	return;
}

void login(t_client client, char *s) 
{
	char *nick = get_word(s);
	char *passwd = get_word(s);
	t_room room;
	t_client offline_client;
	if (strlen(nick) == 0 || strlen(passwd) == 0 || strlen(nick) > 30) {
		bad_format(client);
	}
	if (find_client(ALL_ROOM, nick)) {
		sprintf(msg, "### Клиент %s уже подключён к чату\n", nick);
		print_to(client, msg);
		return;
	}
	if ((offline_client = find_offline_client(nick)) == NULL) {
		print_to(client, "### Вы должны зарегистрироваться\n");
		return;
	}
	if (strcmp(offline_client->passwd, passwd) != 0) {
		print_to(client, "### Неверный пароль\n");
		return;
	}
	rm_client(OFFLINE, offline_client);
	client->nick = offline_client->nick;
	client->passwd = offline_client->passwd;
	free(offline_client);
	sprintf(msg, "### Вы успешно авторизовались.Ваш ник: %s\n", nick);
	print_to(client, msg);
	sprintf(msg, "*** Клиент %s зашёл в чат\n", nick);
	for (room = get_hall(); room; room = room->next)
		print_to_room(room, client, msg);
	return;
}

void whisper(t_client client, char *s)
{
	char *nick;
	t_client recv_client;
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	nick = get_word(s);
	if (strlen(nick) == 0) {
		bad_format(client);
		return;
	}
	recv_client = find_client(NULL, nick);
	if (recv_client) {
		sprintf(msg, "* <%s> %s\n", client->nick, s);
		print_to(recv_client, msg);
	} else {
		if (find_offline_client(nick)) {
			sprintf(msg, "### Пользователь %s не в сети\n", nick);
			print_to(client, msg);
		} else {
			sprintf(msg, "### Пользователь %s не зарегистрирован\n", nick);
			print_to(client, msg);
		}
	}
	return;	
}

void help(t_client client)
{
	char *help = (char *) malloc(MAXSIZE);
	strcpy(help, "### my_chat. Help\n");
	print_to(client, help);
	strcpy(help, "### Команды для не зарегистрированных пользователей\n");
	print_to(client, help);
	strcpy(help, "### /help помощь\n");
	print_to(client, help);
	strcpy(help, "### /reg <nick> <password> регистрация\n");
	print_to(client, help);
	strcpy(help, "### /login <nick> <password> авторизация\n");
	print_to(client, help);
	strcpy(help, "### /leave [<last message>] выход\n");
	print_to(client, help);
	strcpy(help, "### Команды для зарегистрованных пользователей\n");
	print_to(client, help);
	strcpy(help, "### /nick <new_nick> сменить ник\n");
	print_to(client, help);
	strcpy(help, "### /passwd <new_password> сменить пароль\n");
	print_to(client, help);
	strcpy(help, "### /whisper <nick> <message> написать личное сообщение\n");
	print_to(client, help);
	strcpy(help, "### /users [<room>] посмотреть клиентов в комнате или чате\n");
	print_to(client, help);
	strcpy(help, "### /room <room_name> зайти в комнату или создать новую\n");
	print_to(client, help);
	strcpy(help, "### /rooms посмотреть список всех комнат\n");
	print_to(client, help);
	strcpy(help, "### [message] написать всей комнате(кроме прихожей)\n");
	print_to(client, help);
	strcpy(help, "### Команды для операторов\n");
	print_to(client, help);
	strcpy(help, "### /ban <nick> забанить пользователя в комнате\n");
	print_to(client, help);
	strcpy(help, "### /kick <nick> кикнуть пользователя\n");
	print_to(client, help);
	strcpy(help, "### /topic <topic> изменить тему комнаты\n");
	print_to(client, help);
	strcpy(help, "### Команды для администраторов комнат\n");
	print_to(client, help);
	strcpy(help, "### /rmroom удалить комнату\n");
	print_to(client, help);
	strcpy(help, "### /op <nick> дать полномочия модератором\n");
	print_to(client, help);
	strcpy(help, "### /deop <nick> снять полномочия модератора\n");
	print_to(client, help);
	free(help);
	return;
}

void users(t_client client, char *s)
{
	char *name;
	t_room room;
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	if (strlen(name = get_word(s)) != 0) {
		room = find_room(name);
		if (room == NULL) {
			sprintf(msg, "### Комната %s не найдена\n", name);
			print_to(client, msg);
		} else
			print_user_room(room, client);
		return;
	}
	for (room = get_hall(); room; room = room->next)
		print_user_room(room, client);
	return;
}

void leave (t_room room, t_client client, char *s)
{
	t_room tmp;
	if (client->nick) {
		sprintf(msg, "*** Пользователь %s отключается. %s\n", client->nick, s);
		for (tmp = get_hall(); tmp; tmp = tmp->next)
			print_to_room(room, client, msg);
	}
	close(client->sock);
	rm_client(room, client);
	add_client(OFFLINE, client);
	return;
}

void enter()
{
	t_client client = (t_client) malloc(sizeof(*client));
	client->sock =  accept(get_listen_sock(), NULL, NULL);
	client->nick = NULL;
	add_client(get_hall(), client);
	print_to(client, "### Добро Пожаловать!!! Зарегистрируйтесь или авторизуйтесь\n");
	print_to(client, "### Для получения справки введите /help\n");
	print_to(client, "> ");
	return;
}

void enter_room(t_room old_room, t_client client, char *s)
{
	char *msg = (char *) malloc(MAXSIZE);
	char *name = get_word(s);
	t_room new_room;
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	if (strlen(name) == 0) {
		bad_format(client);
		return;
	}
		
	if ((new_room = find_room(name)) == NULL) {
		new_room = make_room(client->nick, name);
		add_room(new_room);
		rm_client(old_room, client);
		add_client(new_room, client);
		sprintf(msg, "### Вы создали новою комнату %s.Наберите /help\n", name);
		print_to(client, msg);
		return;
	}
	if (find_nick(BAN, new_room, client->nick)) {
		print_to(client, "### Вы забанены в этой комнате\n");
		return;
	}
		
	rm_client(old_room, client);
	add_client(new_room, client);
	sprintf(msg, "### Вы зашли в комнату %s. Ваш статус: %s\n", name, 
		get_status(new_room, client->nick)
	);
	print_to(client, msg);
	sprintf(msg, "*** Клиент %s зашёл в комнату\n", client->nick);
	print_to_room(new_room, client, msg);
	return;
}

void rooms(t_client client)
{
	t_room room;
	char *msg = (char *) malloc(MAXSIZE);
	if (client->nick == NULL) {
		no_name(client);
		return;
	}
	for (room = get_hall(); room; room = room->next) {
		sprintf(msg, "### Комната: %s. Тема: %s. Количество пользователей: %d\n", 
				room->name, room->topic ? room->topic : "Нет темы",
				get_num_clients(room)
		);
		print_to(client, msg);
	}
}

void op(t_room room, t_client admin, char *s)
{
	char *nick = get_word(s);
	t_client client;
	if (admin->nick == NULL) {
		no_name(admin);
		free(nick); return;
	}
	if (strlen(nick) == 0) {
		bad_format(admin);
		free(nick); return;
	}
	if (room == get_hall() || strcmp(admin->nick, room->admin)!= 0) {
		print_to(admin, "### Вы не являетесь админом этой комнаты\n");
		free(nick); return;
	}
	if (!(client = find_client(ALL_ROOM, nick)) &&
		!find_offline_client(nick)
	) {
		sprintf(msg, "### Клиент %s не найден\n", nick);
		print_to(admin, msg);
		free(nick); return;
	}
	if (!find_nick(MODER, room, nick)) {
		add_nick(MODER, room, nick);
		sprintf(msg, "*** Клиент %s стал модератором\n", nick);
		print_to_room(room, client, msg);
		if (client)	{
			sprintf(msg, "### Вы стали модератором комнаты %s\n", room->name);
			print_to(client, msg);
		}
	} else {
		sprintf(msg, "### Клиент %s уже модератор этой комнаты\n",nick);
		print_to(admin, msg);
	}
	free(nick);	return;
}

void deop(t_room room, t_client admin, char *s)
{
	char *nick = get_word(s);
	if (admin->nick == NULL) {
		no_name(admin);
		free(nick); return;
	}
	if (strlen(nick) == 0) {
		bad_format(admin);
		free(nick); return;
	}
	if (room == get_hall() || strcmp(admin->nick, room->admin)!= 0) {
		print_to(admin, "### Вы не являетесь админом этой комнаты\n");
		free(nick); return;
	}
	if (strcmp(admin->nick, nick) == 0) {
		print_to(admin, "### Вы не можете снять с себя полномочия модератора\n");
		free(nick); return;
	}
	if (find_nick(MODER, room, nick)) {
		t_client client;
		rm_nick(MODER, room, nick);
		sprintf(msg, "*** Клиент %s перестал быть модератором\n", nick);
		print_to_room(room, NULL, msg);
		if ((client = find_client(ALL_ROOM, nick)) != NULL) {
			sprintf(msg, "### Вы перестали быть модератором в комнате %s\n", room->name);
			print_to(client, msg);
		}	
	} else {
		sprintf(msg, "### Ника %s нет в списке модераторов\n", nick);
		print_to(admin, msg);
	}
	free(nick); return;
}

void ban(t_room room, t_client moder, char *s)
{
	char *nick = get_word(s);
	t_client client ;
	if (moder->nick == NULL) {
		no_name(moder);
		free(nick); return;
	}
	if (strlen(nick) == 0) {
		bad_format(moder);
		free(nick); return;
	}
	if (room == get_hall() || !find_nick(MODER, room, moder->nick)) {
		print_to(moder, "### Вы не являетесь модератором этой комнаты\n");
		free(nick); return;
	}
	if (find_nick(MODER, room, nick)) {
		print_to(moder, "### Вы не можете забанить модератора\n");
		free(nick); return;
	}

	if (!(client = find_client(ALL_ROOM, nick)) &&
		!find_offline_client(nick)
	) {
		sprintf(msg, "### Клиент %s не найден\n", nick);
		print_to(moder, msg);
		free(nick); return;
	}
	
	if (!find_nick(BAN, room, nick)) {
		add_nick(BAN, room, nick);
		sprintf(msg, "*** Клиент %s забанен. %s\n", nick, s);
		print_to_room(room, NULL, msg);
		if (client) {
			sprintf(msg, "### Вас забанил %s в комнате %s. %s", moder->nick, room->name, s);
			print_to(client, msg);
		}
		if ((client = find_client(room, nick)) != NULL) {
			rm_client(room, client);
			add_client(get_hall(), client);
		}
	} else {
		sprintf(msg, "### Клиент %s уже забанен\n", nick);
		print_to(moder, msg);
	}	
	return;
}

void unban(t_room room, t_client moder, char *s)
{
	char *nick = get_word(s);
	t_client client = find_client(ALL_ROOM, nick);
	if (moder->nick == NULL) {
		no_name(moder);
		return;
	}
	if (strlen(nick = get_word(s)) == 0) {
		bad_format(moder);
		return;
	}
	if (room == get_hall() || !find_nick(MODER, room, moder->nick)) {
		print_to(moder, "### Вы не являетесь модератором этой комнаты\n");
		return;
	}
	if (find_nick(BAN, room, nick)) {
		rm_nick(BAN, room, nick);
		sprintf(msg, "*** Клиент %s разбанен. %s\n", nick, s);
		print_to_room(room, client, msg);
		if (client) {
			sprintf(msg, "### Вы разбанены в комнате %s. %s\n", room->name, s);
			print_to(client, msg);
		}		
	} else {
		sprintf(msg, "### Клиента %s нет в бан-листе\n", nick);
		print_to(moder, msg);
	}	
	return;
}

void kick(t_room room, t_client moder, char *s)
{
	char *nick = get_word(s);
	t_client client;
	if (moder->nick == NULL) {
		no_name(moder);
		free(nick); return;
	}
	if (strlen(nick) == 0) {
		bad_format(moder);
		free(nick); return;
	}
	if (!find_nick(MODER, room, moder->nick)) {
		print_to(moder, "### Вы не являетесь модератором этой комнаты\n");
		free(nick); return;
	}
	client = find_client(room, nick);
	if (client == NULL) {
		sprintf(msg, "### Клиента %s нет в комнате\n", nick);
		print_to(moder, msg);
		free(nick); return;
	}
	sprintf(msg, "*** Клиент %s был кикнут\n", nick);
	print_to_room(room, client, msg);
	sprintf(msg, "### Вас кикнул модератор %s\n", moder->nick);
	write(client->sock, msg, strlen(msg) + 1);
	print_to(client, msg);
	rm_client(room, client);
	add_client(get_hall(), client);
	free(nick); return;
}

void topic(t_room room, t_client moder, char *s)
{
	if (moder->nick == NULL) {
		no_name(moder);
		return;
	}
	if (strlen(s) == 0) {
		bad_format(moder);
		return;
	}
	if (!find_nick(MODER, room, moder->nick)) {
		print_to(moder, "### Вы не являетесь модератором этой комнаты\n");
		return;
	}
	free(room->topic);
	room->topic = (char *) malloc(strlen(s) + 1);
	strcpy(room->topic, s);
	sprintf(msg, "*** Модератор %s изменил тему комнаты: %s\n", moder->nick, s);
	print_to_room(room, moder, msg);
	sprintf(msg, "### Вы изменили тему: %s\n", s);
	print_to(moder, msg);
	return;
}

void rmroom(t_room room, t_client admin, char *s)
{
	t_room hall = get_hall();
	t_client client;
	if (admin->nick == NULL) {
		no_name(admin);
		return;
	}
	if (room->admin == NULL || strcmp(admin->nick, room->admin) != 0) {
		print_to(admin, "### Вы не администратор этой комнаты\n");
		return;
	}
	sprintf(msg, "*** Комната удалена. %s\n", s);
	for (client = room->client_list; client; client = client->next) {
		print_to(client, msg);
		rm_client(room, client);
		add_client(hall, client);
	}
	rm_room(room);
	return;
}	
