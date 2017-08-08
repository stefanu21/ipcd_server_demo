#ifndef IPCD_H
#define IPCD_H

#include <sys/types.h>
#include <unistd.h>
#include "msg.h"

struct ipcd_head_t
{
        pid_t pid;
        enum msg_type_t type;
        unsigned long msg_len;
};

typedef struct ipcd_obj_t ipcd_obj;

ipcd_obj *ipcd_obj_new(void **msg, enum msg_type_t type, size_t msg_len);
void ipcd_obj_free(ipcd_obj *ipcd_obj);

int ipcd_obj_send(int fd, ipcd_obj *obj);

#endif /* IPCD_H */

