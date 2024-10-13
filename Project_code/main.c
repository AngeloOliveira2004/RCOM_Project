// Main file of the serial port project.
// NOTE: This file must not be changed.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "application_layer.h"

#define N_TRIES 3
#define TIMEOUT 4


// Arguments:
//   $1: /dev/ttySxx
//   $2: baud rate
//   $3: tx | rx
//   $4: filename
int main(int argc, char *argv[])
{ 
    if(argc != 7){
        printf("Usage: %s <serial_port_addr> <operation_mode (w/r)> <baudrate> <nRetries> <timeout> <file_name>\n",argv[0]);
        return -1;
    }

    if(argv[1] != "/dev/ttyS10" || argv[1] != "/dev/ttyS11"){
        printf("Invalid serial port address, please insert '/dev/ttyS10' for transmitter and 'dev/ttyS11' for receiver\n");
        return -1;
    }

    if(strcmp(argv[2],"w") != 0 || strcmp(argv[2],"r") != 0){
        printf("Invalid operation mode, please insert 'w' for sender and 'r' to receiver\n");
        return -1;
    }

    if(atoi(argv[3]) < 0 || argv[3] == NULL){
        printf("Invalid baudrate, please insert a number higher then 0\n");
        return -1;
    }

    if(atoi(argv[4]) < 0 || argv[4] == NULL){
        printf("Invalid number of retries, please insert a number higher then 0\n");
        return -1;
    }

    if(atoi(argv[5]) < 0 || argv[5] == NULL){
        printf("Invalid timeout value, please insert a number higher then 0\n");
        return -1;
    }

    // Change this
    
    const char *serialPort = argv[1];
    const int baudrate = atoi(argv[2]);
    const char *role = argv[3];
    const char *filename = argv[4];

    // Validate baud rate
    switch (baudrate) {
        case 1200:
        case 1800:
        case 2400:
        case 4800:
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
            break;
        default:
            printf("Unsupported baud rate (must be one of 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200)\n");
            exit(2);
    }

    // Validate role
    if (strcmp("tx", role) != 0 && strcmp("rx", role) != 0) {
        printf("ERROR: Role must be \"tx\" or \"rx\"\n");
        exit(3);
    }

    printf("Starting link-layer protocol application\n"
           "  - Serial port: %s\n"
           "  - Role: %s\n"
           "  - Baudrate: %d\n"
           "  - Number of tries: %d\n"
           "  - Timeout: %d\n"
           "  - Filename: %s\n",
           serialPort,
           role,
           baudrate,
           N_TRIES,
           TIMEOUT,
           filename);

    applicationLayer(serialPort, role, baudrate, N_TRIES, TIMEOUT, filename);

    return 0;
}
