// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "globals.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = FALSE;
int alarmCount = 0;

#define MSG_SIZE 1000

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }

    connectionParameters.role == LlTx ? llwrite(NULL, 0) : llread(NULL);

    unsigned char message[MSG_SIZE];

    //Implement the state machine

    switch (connectionParameters.role)
    {
    case LlTx:
        
        while (connectionParameters.nRetransmissions > 0){

            if (alarmEnabled == FALSE) {
                alarm(3);
                signal(SIGALRM, alarmHandler);

                message[0] = FLAG;
                message[1] = A1;
                message[2] = SET;
                message[3] = A1 ^ SET;
                message[4] = FLAG;

                llwrite(message, 5);
                // Resend the SET message after the alarm triggers
                /*bytes_written = write(fd, set_message, MSG_SIZE);
                if (bytes_written != MSG_SIZE) {
                    perror("write");
                    exit(-1);
                }
                printf("Resent %d bytes\n", bytes_written);
                */
                alarmEnabled = TRUE;
                connectionParameters.nRetransmissions--;
            }
            
        }
        {
            /* code */
        }
        
        // Send SET frame

        // Wait for UA frame
    

        break;
    case LlRx:
        /* code */
        break;
    default:
        printf("Invalid role\n");
        exit(-1);
    }

    if (connectionParameters.role == LlTx) {
        // Send SET frame
        unsigned char setFrame[5] = {FLAG, A1, SET , A1 ^ SET, FLAG};
        llwrite(setFrame, 5);

        // Wait for UA frame
        unsigned char uaFrame[5];
        readSerialPort(uaFrame, 5);
        if (uaFrame[2] != UA) {
            return -1;
        }
    } else {
        // Wait for SET frame
        unsigned char setFrame[5];
        readSerialPort(setFrame, 5);
        if (setFrame[2] != SET) {
            return -1;
        }

        // Send UA frame
        unsigned char uaFrame[5] = {FLAG, A1, UA, A1 ^ UA, FLAG};
        llwrite(uaFrame, 5);
    }

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    //Filter the Packet

    unsigned char *packetFiltered = malloc(MAX_PAYLOAD_SIZE);
    int packetFilteredSize = 0;

    for (int i = 0; i < MAX_PAYLOAD_SIZE; i++)
    {
        if (packet[i] == FLAG)
        {
            if (packet[i + 1] == FLAG)
            {
                packetFiltered[packetFilteredSize] = packet[i];
                packetFilteredSize++;
                i++;
            }
            else
            {
                break;
            }
        }
        else
        {
            packetFiltered[packetFilteredSize] = packet[i];
            packetFilteredSize++;
        }
    }

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    

    int clstat = closeSerialPort();
    return clstat;
}
