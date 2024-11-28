#include "server.h"

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

int closeConnection(int sockfd) {
    if (close(sockfd) < 0) {
        perror("close()");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int readResponseCode(int sockfd) {
    char response[4];
    if (read(sockfd, response, 3) <= 0) {
        perror("Error reading response code");
        return -1;
    }
    response[3] = '\0'; // Null-terminate
    return atoi(response); // Convert to integer
}  

int readResponse(int sockfd, char **response, int *responseSize) {
    char buffer[MAX_LENGTH];
    int totalBytes = 0;
    int isMultiLine = 0;

    // Read response code and determine type
    int code = readResponseCode(sockfd);
    if (code < 0) return -1;

    char byte;
    while (1) {
        if (read(sockfd, &byte, 1) <= 0) {
            perror("Error reading byte");
            return -1;
        }

        buffer[totalBytes++] = byte;

        // Check if the line ends (CRLF)
        if (byte == LF && totalBytes >= 2 && buffer[totalBytes - 2] == CR) {
            if (!isMultiLine) {
                // Check for multi-line continuation
                if (buffer[3] == '-') {
                    isMultiLine = 1;
                } else {
                    break; // End of single-line response
                }
            } else {
                // Multi-line ends when the code is followed by a space
                if (strncmp(buffer, "220 ", 4) == 0) {
                    break;
                }
            }
        }
    }

    memcpy(*response, buffer, totalBytes);
    (*response)[totalBytes] = '\0'; 
    *responseSize = totalBytes;
    return code;
}


int initializeCon(int sockfd, char **response, int *responseSize) {
    int code = readResponse(sockfd, response, responseSize);
    if (code < 0) {
        fprintf(stderr, "Failed to initialize connection\n");
        return -1;
    }
    if (code == 220) {
        printf("Connection initialized: %s\n", *response);
        return 0;
    }
    fprintf(stderr, "Unexpected response: %s\n", *response);
    return -1;
}


int authenticate(int socket , struct URL *SERVER_URL){
    char *response = malloc(MAX_LENGTH);
    int responseSize = 0;

    //USER
    char command[MAX_LENGTH];
    snprintf(command, MAX_LENGTH, SERVER_URL->user);
    if (write(socket, command, strlen(command)) <= 0) {
        perror("Error sending USER command");
        return -1;
    }

    if(readResponse(socket , &response , &responseSize) == 0){
        fprintf(stderr, "Error reading response.\n");
        exit(EXIT_FAILURE);
    }

    printf("Response: %s\n", response);

    //PASSWORD
    snprintf(command, MAX_LENGTH, SERVER_URL->password);
    write(socket , command , strlen(command));

    if(readResponse(socket , &response , &responseSize) == 0){
        fprintf(stderr, "Error reading response.\n");
        exit(EXIT_FAILURE);
    }

    printf("Response: %s\n", response);

    free(response);
    return 0;
}


int passiveMode(int sockfd , struct URL *SERVER_URL , char * newPort , char * newIP){ //Server IP addr. Server IP port for file transfer = 256×198 + 138 = 50826
    char *response = malloc(MAX_LENGTH);
    int responseSize = 0;

    write(sockfd , "PASV\r\n" , 6);

    if(readResponse(sockfd , &response , &responseSize) != PASSIVE_MODE){
        fprintf(stderr, "Error reading response.\n");
        exit(EXIT_FAILURE);
    }

    printf("Response: %s\n", response);

    return PASSIVE_MODE;
}