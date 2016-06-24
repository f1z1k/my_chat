#include "cmd.h"
#include "net.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
	signal(SIGINT, stop_server);	
	signal(SIGTERM, stop_server);
	start_server(argv[1]);
	while (1) {
		make_ready_sock_set();
		if (is_new_client())
			enter();
		process_clients();
	}
	return 0;
}
