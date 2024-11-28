#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "syscall.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "netdb.h"
#include <regex.h>

#define MAX_LENGTH 1024

#define FALSE 0
#define TRUE 1

#define CR '\r'
#define LF '\n'
#define CRLF '\r\n'

struct URL
{
    char protocol[MAX_LENGTH];
    char domain[MAX_LENGTH];
    char path[MAX_LENGTH];
    char address[MAX_LENGTH];
    char user[MAX_LENGTH];
    char password[MAX_LENGTH];
    char ip[MAX_LENGTH];
    int  port;
};

enum ResponseCodes{
    OK = 220,
    ASK_PASSWORD = 331,
    LOGIN_SUCCESSFUL = 230,
    DIR_CHANGED = 250,
    FILE_SIZE = 213,
    PASSIVE_MODE = 227,
    TRANSFER_COMPLETE = 226, //Final reply (1st digit of code > 1)
    BINARY_MODE = 150, //in order to transfer the file, will only be used in the state machine of file transfer
    END = 221,
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

struct URL *parseUrl(const char *url);

void getIP(char *hostname , char *ip);

void printURL(struct URL *url);

int readByte(int socket , char *byte , const int size);