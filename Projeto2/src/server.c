#include "server.h"

/**      (C)2000-2021 FEUP
 *       tidy up some includes and parameters
 * 
 *       Connect to the server.
 *       Adaptation from clientTCP.c , given by teachers.
 * */

int connectToServer(struct URL *SERVER_URL) {
    int SERVER_PORT = SERVER_URL->port;
    char *SERVER_IP = SERVER_URL->ip;

    printf("Connecting to server %s:%d\n", SERVER_IP, SERVER_PORT);

    int sockfd;
    struct sockaddr_in server_addr;

    /* Server address handling */
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); /* 32-bit Internet address, network byte ordered */
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

    printf("Reading response...\n");

    // Read response code and determine type
    int code = readResponseCode(sockfd);

    printf("Response code: %d\n", code);

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
                printf("Multi-line continuation\n");
                for(int i = 0 ; i < 4 ; i++){
                    printf("%c" , buffer[i]);
                }
                memset(buffer, 0, MAX_LENGTH);
                totalBytes = 0;
                // Multi-line ends when the code is followed by a space
                if (strncmp(buffer, "220 ", 4) == 0) {
                    printf("End of multi-line response\n");
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
        return OK;
    }
    fprintf(stderr, "Unexpected response: %s\n", *response);
    return -1;
}


int authenticate(int socket , struct URL *SERVER_URL){
    char *response = malloc(MAX_LENGTH);
    int responseSize = 0;

    printf("Authenticating...\n");

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


int passiveMode(int sockfd, struct URL *SERVER_URL, char **newPort, char **newIP) {
    char *response = malloc(MAX_LENGTH);
    int responseSize = 0;

    // Send PASV command
    write(sockfd, "PASV\r\n", 6);

    // Read the response from the server
    if (readResponse(sockfd, &response, &responseSize) != PASSIVE_MODE) {
        fprintf(stderr, "Error reading response.\n");
        free(response);  // Don't forget to free memory
        return -1;
    }

    printf("Response: %s\n", response);

    // Find the part of the response that contains the IP and port information
    char *start = strstr(response, "(");  // Find the start of the IP/port
    char *end = strstr(response, ")");    // Find the end of the IP/port

    if (start == NULL || end == NULL) {
        fprintf(stderr, "Error: Invalid PASV response format.\n");
        free(response);
        return -1;
    }

    // Extract the part inside the parentheses
    start++;  // Skip the '('
    *end = '\0';  // Null-terminate at ')'

    // Now `start` contains the string like "193,137,29,15,198,138"
    int ip1, ip2, ip3, ip4, portHigh, portLow;
    if (sscanf(start, "%d,%d,%d,%d,%d,%d", &ip1, &ip2, &ip3, &ip4, &portHigh, &portLow) != 6) {
        fprintf(stderr, "Error parsing PASV response.\n");
        free(response);
        return -1;
    }

    // Construct the new IP address and port number
    snprintf(*newIP, MAX_LENGTH, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
    int port = 256 * portHigh + portLow;
    snprintf(*newPort, MAX_LENGTH, "%d", port);

    // Clean up and return
    free(response);
    return PASSIVE_MODE;
}


int downloadFile(const int serverSocket, const int clientSocket, const char *filename) {
    // Send RETR command
    char command[MAX_LENGTH];
    snprintf(command, MAX_LENGTH, "RETR %s\r\n", filename);
    if (write(serverSocket, command, strlen(command)) <= 0) {
        perror("Error sending RETR command");
        return -1;
    }

    // Read the response from the server
    char *response = malloc(MAX_LENGTH);
    int responseSize = 0;
    if (readResponse(serverSocket, &response, &responseSize) != BINARY_MODE) {
        fprintf(stderr, "Error reading response.\n");
        free(response);  // Don't forget to free memory
        return -1;
    }

    printf("Response: %s\n", response);

    // Open the file for writing
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file for writing");
        free(response);
        return -1;
    }

    // Read the file contents from the server
    char buffer[MAX_LENGTH];
    int bytesRead;
    while ((bytesRead = read(serverSocket, buffer, MAX_LENGTH)) > 0) {
        if (write(clientSocket, buffer, bytesRead) <= 0) {
            perror("Error writing to client socket");
            fclose(file);
            free(response);
            return -1;
        }
        fwrite(buffer, 1, bytesRead, file);
    }

    // Close the file and clean up
    fclose(file);
    free(response);
    return TRANSFER_COMPLETE;
}