#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syscall.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"

struct URL
{
    char *protocol;
    char *domain;
    char *port;
    char *path;
};

struct URL *parse_url(char *url);