#include "server.h"

#define h_addr h_addr_list[0]

/*
    * Get the IP address from hostname.
    @param hostname - Hostname to get IP address.
    
    Adaptation from getIP.c , given by teachers.
*/
void getIP(char *hostname , char *ip) {
    struct hostent *h;

    // Get host entry for the hostname

    fprintf(stderr, "Hostname: %s\n", hostname);

    hostname = "google.com";

    if ((h = gethostbyname(hostname)) == NULL) {
        herror("gethostbyname()");
        exit(-1);
    }

    // Extract the first address (in network byte order) from the hostent structure
    struct in_addr *addr = (struct in_addr *)h->h_addr;

    // Print host name for debugging
    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*addr));
    
    //ip = inet_ntoa(*addr);
    strcpy(ip, inet_ntoa(*addr));

    // Return the IP address as an integer in network byte order
    return;
}

struct URL *parseUrl(const char *url) {
    if (url == NULL) {
        fprintf(stderr, "Error: URL cannot be NULL.\n");
        return NULL;
    }

    // Regex pattern for URL parsing
    const char *pattern = "^(ftp)://([^:@]+:[^:@]+@)?([^:/]+)(:[0-9]+)?(/.*)?$";
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Error: Could not compile regex.\n");
        return NULL;
    }

    // Match regex
    regmatch_t matches[6]; // 1 for each group in the regex
    if (regexec(&regex, url, 6, matches, 0) != 0) {
        fprintf(stderr, "Error: Invalid URL format.\n");
        regfree(&regex);
        return NULL;
    }

    // Allocate memory for URL struct
    struct URL *parsed_url = malloc(sizeof(struct URL));
    if (parsed_url == NULL) {
        perror("malloc");
        regfree(&regex);
        return NULL;
    }

    // Extract protocol
    snprintf(parsed_url->protocol, sizeof(parsed_url->protocol), "%.*s",
             (int)(matches[1].rm_eo - matches[1].rm_so), url + matches[1].rm_so);

    // Extract domain
    snprintf(parsed_url->domain, sizeof(parsed_url->domain), "%.*s",
             (int)(matches[3].rm_eo - matches[3].rm_so), url + matches[3].rm_so);

    // Extract port if present
    if (matches[4].rm_so != -1) {
        char port_str[6] = {0};
        snprintf(port_str, sizeof(port_str), "%.*s",
                 (int)(matches[4].rm_eo - matches[4].rm_so - 1), // Skip the ':'
                 url + matches[4].rm_so + 1);
        parsed_url->port = atoi(port_str);
    } else {
        parsed_url->port = 21; // Default FTP port
    }

    // Extract path if present
    if (matches[5].rm_so != -1) {
        snprintf(parsed_url->path, sizeof(parsed_url->path), "%.*s",
                 (int)(matches[5].rm_eo - matches[5].rm_so), url + matches[5].rm_so);
    } else {
        strcpy(parsed_url->path, "/");
    }

    // Copy domain to address
    snprintf(parsed_url->address, sizeof(parsed_url->address), "%s", parsed_url->domain);

    // Resolve domain to IP address
    getIP(parsed_url->domain , parsed_url->ip);

    regfree(&regex);
    return parsed_url;
}

void printURL(struct URL *url){
    printf("Protocol: %s\n", url->protocol);
    printf("Domain: %s\n", url->domain);
    printf("Path: %s\n", url->path);
    printf("Port: %d\n", url->port);
    printf("Address: %s\n", url->address);
    printf("IP: %s\n", url->ip);
    printf("User %s\n" , url->user);
    printf("Password %s\n" , url->password);
}

int readByte(int socket , char *byte , const int size){
    int readBytes = read(socket , byte , size);

    if(readBytes < 0){
        perror("read");
        exit(EXIT_FAILURE);
    }

    return readBytes;
}

int main(int argc, char *argv[]) {
    
    if(argc != 2){
        printf("Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>\n");
        exit(-1);
    }

    size_t input_length = strlen(argv[1]);

    printf("Input URL: %s\n", argv[1]);
    printf("URL Length: %zu characters\n", input_length);

    struct URL *parsed_url = parseUrl(argv[1]);

    printURL(parsed_url);

    int serverSocketFD = connectToServer(parsed_url);

    char *response = malloc(MAX_LENGTH);
    if (!response) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    int responseSize = 0;

    if(initializeCon(serverSocketFD , &response , &responseSize) == OK){
        printf("Response: %s\n", response);
    }else{
        fprintf(stderr, "Error initializing connection.\n");
        exit(EXIT_FAILURE);
    }

    authenticate(serverSocketFD , parsed_url);

    struct URL *newURL = malloc(sizeof(struct URL));
    char * newIP;
    char * newPort;

    passiveMode(serverSocketFD , parsed_url , &newPort , &newIP);

    newURL->port = atoi(newPort);
    strcpy(newURL->ip , newIP);

    printf("New IP: %s\n" , newURL->ip);
    printf("New Port: %d\n" , newURL->port);

    int clientSocketFD = connectToServer(newURL);

    char * fileName = parsed_url->path;
    int fileNameSize = 0;

    for(int i = strlen(parsed_url->path) ; i > 0 ; i--){
        if(parsed_url->path[i] == '/'){
            fileNameSize = strlen(parsed_url->path) - i;
            break;
        }

        fileName[fileNameSize++] = parsed_url->path[i];
    }

    downloadFile(serverSocketFD , clientSocketFD , fileName);

    free(parsed_url);

    closeConnection(clientSocketFD);
    closeConnection(serverSocketFD);

    //example url: ftp://username:password@<domain>/<path>
    //example url: ftp.netlab.fe.up.pt/pub/files/file.txt
    //example url: ftp://username:password@ftp.netlab.fe.up.pt/pub/files/file.txt

    /*
    Protocol (ftp):
    Extract everything before ://.

    User and Password (username:password):
    Extract the portion between :// and @. Split into user and password using :.

    Domain (ftp.netlab.fe.up.pt):
    Extract everything between @ and the first /. This is the domain.

    Path (/pub/files/file.txt):
    Extract everything after the first /.

    Address (ftp.netlab.fe.up.pt or IP):
    This duplicates domain unless resolved to an IP address.

    IP (193.136.212.249):
    Perform a DNS lookup on domain to resolve its IP address.

    Port (21):
    The default FTP port is 21. If the domain includes a port (e.g., ftp.netlab.fe.up.pt:2121), extract and use that.
    */

    return 0;
}
