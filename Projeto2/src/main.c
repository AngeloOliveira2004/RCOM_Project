#include "aux.h"

#define h_addr h_addr_list[0]

/*
    * Get the IP address from hostname.
    @param hostname - Hostname to get IP address.
    
    Adaptation from getIP.c , given by teachers.
*/
int getIP(char *hostname) {
    struct hostent *h;

    // Get host entry for the hostname
    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    // Extract the first address (in network byte order) from the hostent structure
    struct in_addr *addr = (struct in_addr *)h->h_addr;

    // Print host name for debugging
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*addr));

    // Return the IP address as an integer in network byte order
    return addr->s_addr;
}

/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * 
 *       Connect to the server.
 *       Adaptation from clientTCP.c , given by teachers.
 * */

int connectToServer(struct URL *SERVER_URL) {
    int SERVER_PORT = SERVER_URL->port;
    char *SERVER_ADDR = SERVER_URL->address;

    int sockfd;
    struct sockaddr_in server_addr;

    /* Server address handling */
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR); /* 32-bit Internet address, network byte ordered */
    server_addr.sin_port = htons(SERVER_PORT);           /* Server TCP port must be network byte ordered */

    /* Open a TCP socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sockfd); /* Clean up before exiting */
        exit(EXIT_FAILURE);
    }

    /* Return the socket descriptor for further use */
    return sockfd;
}

struct URL *parse_url(char *url) {
    if (url == NULL) {
        fprintf(stderr, "Error: URL cannot be NULL.\n");
        return NULL;
    }

    struct URL *parsed_url = malloc(sizeof(struct URL));
    if (parsed_url == NULL) {
        perror("malloc");
        return NULL;
    }

    parsed_url->protocol = malloc(8);  // "ftp://" is 6 chars, leaving room for null terminator
    parsed_url->domain = malloc(128);
    parsed_url->path = malloc(256);

    if (!parsed_url->protocol || !parsed_url->domain || !parsed_url->path) {
        perror("malloc");
        free(parsed_url->protocol);
        free(parsed_url->domain);
        free(parsed_url->path);
        free(parsed_url);
        return NULL;
    }

    // Extract protocol (e.g., "ftp")
    char *protocol = strtok(url, ":");
    if (protocol == NULL) {
        fprintf(stderr, "Error: Invalid URL format (missing protocol).\n");
        free(parsed_url);
        return NULL;
    }
    strcpy(parsed_url->protocol, protocol);

    // Check for "://" after protocol
    if (strncmp(strtok(NULL, ""), "//", 2) != 0) {
        fprintf(stderr, "Error: Invalid URL format (missing // after protocol).\n");
        free(parsed_url);
        return NULL;
    }

    // Extract domain (e.g., "ftp.netlab.fe.up.pt")
    char *domain = strtok(NULL, "/");
    if (domain == NULL) {
        fprintf(stderr, "Error: Domain missing in URL.\n");
        free(parsed_url);
        return NULL;
    }
    strcpy(parsed_url->domain, domain);

    // Extract port if present (e.g., "ftp.netlab.fe.up.pt:21")
    char *port_ptr = strchr(parsed_url->domain, ':');
    if (port_ptr != NULL) {
        *port_ptr = '\0'; // Null-terminate the domain part
        parsed_url->port = atoi(port_ptr + 1); // Extract the port number
    } else {
        parsed_url->port = 21; // Default FTP port
    }

    // Extract path (e.g., "/pub/")
    char *path = strtok(NULL, "");
    if (path == NULL) {
        parsed_url->path = malloc(2);  // Default to "/"
        strcpy(parsed_url->path, "/");
    } else {
        strcpy(parsed_url->path, path);
    }

    // Copy domain as address (may be useful for other purposes)
    parsed_url->address = malloc(strlen(parsed_url->domain) + 1);
    if (parsed_url->address != NULL) {
        strcpy(parsed_url->address, parsed_url->domain);
    }

    // Resolve the domain to IP address
    parsed_url->ip = getIP(parsed_url->domain);
    if (parsed_url->ip == -1) {
        fprintf(stderr, "Error: Could not resolve IP address for domain %s.\n", parsed_url->domain);
        free(parsed_url->protocol);
        free(parsed_url->domain);
        free(parsed_url->path);
        free(parsed_url->address);
        free(parsed_url);
        return NULL;
    }

    return parsed_url;
}

void print_URL(struct URL *url){
    printf("Protocol: %s\n", url->protocol);
    printf("Domain: %s\n", url->domain);
    printf("Path: %s\n", url->path);
    printf("Address: %s\n", url->address);
    printf("IP: %d\n", url->ip);
}

int main(int argc, char *argv[]) {
    
    if(argc != 2){
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    size_t input_length = strlen(argv[1]);

    printf("Input URL: %s\n", argv[1]);
    printf("URL Length: %zu characters\n", input_length);

    struct URL *parsed_url = parse_url(argv[1]);

    print_URL(parsed_url);

    //example url: ftp://username:password@ftp.netlab.fe.up.pt/pub/files/file.txt

    return 0;
}

