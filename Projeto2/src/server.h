#include "aux.h"

int connectToServer(struct URL *SERVER_URL);

int closeConnection(int sockfd);

int readResponseCode(int sockfd);

int readResponse(int sockfd, char **response, int *responseSize);

int initializeCon(int sockfd, char **response, int *responseSize);

int authenticate(int socket , struct URL *SERVER_URL);

int passiveMode(int sockfd , struct URL *SERVER_URL , char * newPort , char * newIP);