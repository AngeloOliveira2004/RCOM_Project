#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syscall.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "netdb.h"

struct URL
{
    char *protocol;
    char *domain;
    char *path;
    char *address;
    char *user;
    char *password;
    int   port;
    int   host;
    int   ip;
};

/*
struct hostent {
    char *h_name;    // Official name of the host.
    char **h_aliases;    // A NULL-terminated array of alternate names for the host.
    int h_addrtype;    // The type of address being returned; usually AF_INET.
    int h_length;    // The length of the address in bytes.
    char **h_addr_list;    // A zero-terminated array of network addresses for the host.
    // Host addresses are in Network Byte Order.
};
*/

struct URL *parse_url(char *url);