#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

#include "ipcd.h"

struct ipcd_obj_t
{
        void *buffer;
        size_t buffer_len;
        void *msg;
};

ipcd_obj *ipcd_obj_new(void **msg, enum msg_type_t type, size_t msg_len)
{
        ipcd_obj *obj = calloc(1, sizeof(*obj));
        struct ipcd_head_t *head = obj->buffer;

        obj->buffer = calloc(1, msg_len + sizeof(*head));
        head = obj->buffer;

        head->pid = getpid();
        head->type = type;
        head->msg_len = msg_len;

        obj->buffer_len = msg_len + sizeof(*head);
        obj->msg = (char *)obj->buffer + offsetof(ipcd_obj, msg);
        *msg = obj->msg;
        return obj;
}


void ipcd_obj_free(ipcd_obj *ipcd_obj)
{
        if(!ipcd_obj)
                return;

        free(ipcd_obj->buffer);
        free(ipcd_obj);
}

int ipcd_obj_send(int fd, ipcd_obj *obj)
{
        size_t tot_send = 0;

        while(tot_send < obj->buffer_len)
        {
                size_t n_write = 0;

                n_write = write(fd, obj->buffer + tot_send, obj->buffer_len - tot_send);

                if(n_write < 0)
                {
                        perror("error write");
                        return -1;
                }

                tot_send += n_write;
        }

        return 0;
}

