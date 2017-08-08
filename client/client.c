#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include "ipcd.h"

#define BACKLOG 10
#define SV_SOCK_PATH	"/tmp/server_socket"




int main(int argc, char *argv[])
{
	struct sockaddr_un addr = { 0 };
	int fds;
	ipcd_obj *ipcd_obj = NULL;

	if((fds  = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
	}

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

	if(connect(fds, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) < 0)
{
	perror("connect");
	close(fds);
	exit(1);
}

	{
		struct connect *connect = NULL;
		ipcd_obj = ipcd_obj_new((void **)&connect, MSG_CONNECT, sizeof(*connect));

		connect->listen_to_type[0] = 1234;
		connect->listen_to_type[1] = 5678;
		snprintf(connect->client_name, sizeof(connect->client_name), "client-%d", getpid());
	
		ipcd_obj_send(fds, ipcd_obj);
		ipcd_obj_free(ipcd_obj);
	}

	struct test *test = NULL;
	ipcd_obj = ipcd_obj_new((void **)&test, MSG_TEST, sizeof(*test));

	snprintf(test->data, sizeof(test->data), "Das ist ein erter Text für die Message Test");
	test->time = time(NULL);

	while(1)
	{
		ipcd_obj_send(fds, ipcd_obj);
		sleep(2);
	}

	ipcd_obj_free(ipcd_obj);
	close(fds);
	exit(EXIT_SUCCESS);
}

