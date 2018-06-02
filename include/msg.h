#ifndef MSG_H
#define MSG_H
#include <time.h>

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

enum msg_type_t
{
        MSG_TEST = 1,
        MSG_CONNECT,
        MSG_TEST2,
};

struct test
{
        char data[128];
        time_t time;
};

struct connect
{
        int listen_to_type[128];
        char client_name[32];
   	 pid_t pid;
};


struct test2
{
        char text[128];
        char text2[8 << 20];
};


#endif /* MSG_H */
