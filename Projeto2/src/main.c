#include "aux.h"

struct URL *parse_url(char *url)
{
    struct URL *parsed_url = malloc(sizeof(struct URL));
    parsed_url->protocol = malloc(8);
    parsed_url->domain = malloc(128);
    parsed_url->port = malloc(8);
    parsed_url->path = malloc(256);

    char *protocol = strtok(url, ":");
    char *slash_slash = strtok(NULL, "/");
    char *domain = strtok(NULL, ":");
    char *port = strtok(NULL, "/");
    char *path = strtok(NULL, "");

    strcpy(parsed_url->protocol, protocol);
    strcpy(parsed_url->domain, domain);
    strcpy(parsed_url->port, port);
    strcpy(parsed_url->path, path);

    return parsed_url;
}

int main(int argc, char *argv[]) {
    // ...
    return 0;
}

