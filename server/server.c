#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/un.h>

#include <ipcd.h>

#define BACKLOG 10
#define SV_SOCK_PATH    "/tmp/server_socket"
#define EPOLL_TIMEOUT_MS	1000
#define MAX_EVENTS	5

//#define logg(FMT, ARGS...)   do{ printf(FMT "\n", ##ARGS);  } while(0);
#define logg(FMT, ARGS...)   do{ } while(0);

struct event_data
{
	int fd;
	void *buffer;
	size_t buffer_len;
	size_t buffer_offset;
	enum msg_type_t type;
	pid_t pid;
};

int main(int argc, char *argv[])
{
	struct sockaddr_un addr;
	int fds, fde, fdc;
	struct epoll_event ev;
	struct epoll_event evlist[MAX_EVENTS];
	struct event_data *user_data;

	if((fde = epoll_create(10)) < 0)
	{
		perror("epoll create");
		exit(1);
	}

	if((fds  = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
	}

	user_data = calloc(1, sizeof(*user_data));

	if(!user_data)
	{
		perror("calloc");
		exit(1);
	}
	user_data->fd = fds;
	ev.data.ptr = (void *)user_data;
	ev.events = EPOLLIN;

	if((epoll_ctl(fde, EPOLL_CTL_ADD, fds, &ev)) < 0)
	{
		perror("epoll_ctl");
		exit(1);
	}
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;

	if(remove(SV_SOCK_PATH) < 0 && errno != ENOENT)
		perror("rm");

	strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

	if(bind(fds, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) < 0)	{
		perror("bind");
		close(fds);
		exit(1);
	}
	
	if((listen(fds, BACKLOG)) < 0)
	{
		perror("listen");
		close(fds);
		exit(1);
	}

	while(1)
	{	

		int i = 0;
		int ready = epoll_wait(fde, evlist, MAX_EVENTS, EPOLL_TIMEOUT_MS);
		if(ready < 0)
		{
			if(errno == EINTR)
				continue; // restart if interrupted by signal
			else
			{
				perror("epoll_wait");
				exit(1);
			}
		}

		if(ready == 0)
		{
			printf("timeout\n");
			continue;
		}
		
		for(i = 0; i < ready; i++)
		{
			ssize_t n_read = 0;
			struct event_data *user_data = (struct event_data *)evlist[i].data.ptr;
			logg("[%d]: fd: %d; events %s\n", i, user_data->fd, (evlist[i].events & EPOLLIN) ? "EPOLLIN" : "unknown");

			if(user_data->fd == fds)
			{
				printf("new client connection\n");
				fdc = accept(fds, NULL, NULL);

				if(fdc < 0)
				{
					perror("accept");
					exit(1);
				}	

		        	fcntl(fdc, F_SETFL, O_NONBLOCK);	
				user_data = calloc(1, sizeof(*user_data));

				if(!user_data)
				{
					perror("alloc user");
					exit(1);
				}

				user_data->fd = fdc;
				ev.data.ptr = (struct event_data *)user_data;
				ev.events = EPOLLIN;

				if(epoll_ctl(fde, EPOLL_CTL_ADD, fdc, &ev) < 0)
				{
					perror("epoll_ctl");
					exit(1);
				}
				continue;
			}

			if(!user_data->buffer_len)
			{
				struct ipcd_head_t head;
				n_read = read(user_data->fd, &head, sizeof(head));

				if(n_read == 0)
					goto disconnect;

				if( n_read < 0)
				{
					perror("read error");
					continue;
				}

				if( n_read != sizeof(head))
				{
					/* TODO: handle partial read of header */
					printf("read to less data %d of %d\n", n_read, sizeof(head));
					continue;
				}

				logg("[%d]: head: type: %d len: %lu\n", head.pid, head.type, head.data_len);
				
				
				user_data->buffer = calloc(1, head.msg_len);
				user_data->buffer_len = head.msg_len;
				user_data->buffer_offset = 0;
				user_data->type = head.type;
				user_data->pid = head.pid;
				if(!user_data->buffer)
				{
					perror("error calloc");
					exit(1);
				}
			}

			n_read = read(user_data->fd, user_data->buffer + user_data->buffer_offset, user_data->buffer_len - user_data->buffer_offset);

			if(n_read == 0)
				goto disconnect;

			if( n_read < 0)
			{
				perror("read error");
				continue;
			}
			user_data->buffer_offset += n_read;

			if( user_data->buffer_offset != user_data->buffer_len)
			{
				logg("[%d]: expect more data %d of %d\n", user_data->pid, user_data->buffer_offset, user_data->buffer_len);
				continue;
			}

			switch(user_data->type)
			{
				case MSG_TEST:
				{
					struct test *msg = (struct test *)user_data->buffer;
					printf("data: %s time %lu\n", msg->data, msg->time);
				}
				break;

				case MSG_CONNECT:
				{
					struct connect *msg = (struct connect *)user_data->buffer;
					printf("MSG_CONNECT %d (%s)\n", user_data->pid, msg->client_name);
					printf("listen to %d and %d\n", msg->listen_to_type[0], msg->listen_to_type[1]);
				}
				break;

				case MSG_TEST2:	
				{
					struct test2 *msg = (struct test2 *)user_data->buffer;

					printf("[%d]: MSG_TEST2\n", user_data->pid);
					logg("[%d]: Text: %s\n", user_data->pid, msg->text);
					logg("[%d]: Text2: %s\n", user_data->pid, msg->text2);
				}
				break;

				default:
					printf("unknown type");
				break;
			}



			/* /TODO: send message to all listening clients */
			free(user_data->buffer);
			user_data->buffer = 0;
			user_data->buffer_len = 0;
			user_data->buffer_offset = 0;
			user_data->pid = 0;
			continue;

disconnect:	
			fdc = user_data->fd;
			printf("[%d]: client %d disconnect\n", user_data->pid, fdc);
			if(epoll_ctl(fde, EPOLL_CTL_DEL, fdc, &ev) < 0)
                        {
                                perror("epoll_ctl");
                                exit(1);
                        }
			
			free(user_data->buffer);
			free(user_data);
			close(fdc);
		} 

	}

	exit(EXIT_SUCCESS);

}
