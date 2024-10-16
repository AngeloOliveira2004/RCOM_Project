// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define BUF_SIZE 256

enum ReadingState{
    START_STATE,
    FLAG_RCV_STATE,
    A_RCV_STATE,
    C_RCV_STATE,
    BCC_OK_STATE,
    DATA_RCV_STATE,
    FOUND_ESC_STATE,
    DISCONNECT_STATE,
    READING_STATE,
    STOP_STATE,
    BCC1_OK_STATE,
    ERROR_STATE,
    DISC_RCV_STATE,
    REJ0_STATE,
    REJ1_STATE 
};

typedef enum{
    I_Frame,
    S_Frame,
    U_Frame,
    Disc_Frame,
    Error_Frame,
    RR0_Frame,
    RR1_Frame,
    REJ0_Frame,
    REJ1_Frame
} FrameType;

typedef enum
{
    LlTx, // Transmitter_Role
    LlRx, // Receiver_Role
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

void alarmHandler(int signal);

int sendCommandBit(int fd , unsigned char A , unsigned char C);

int countZerosFromPacket(unsigned char *packet, int packetSize);

unsigned char * stuffing(unsigned char *frameBuffer, int* size);

unsigned char * destuff(unsigned char* stuffedBuffer, int* size);


char* getReadingStateName(enum ReadingState state);

#endif // _LINK_LAYER_H_
