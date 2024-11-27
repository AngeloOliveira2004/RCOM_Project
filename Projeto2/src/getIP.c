/**
 * Example code for getting the IP address from hostname.
 * tidy up includes
 */
#include "aux.h"

#define h_addr h_addr_list[0]

int main(int argc, char *argv[]) {
    struct hostent *h;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <address to get IP address>\n", argv[0]);
        exit(-1);
    }

    if ((h = gethostbyname(argv[1])) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *) h->h_addr)));

    return 0;
}