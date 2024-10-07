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
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);

    if (fd < 0)
    {
        return -1;
    }

    connectionParameters.role == LlTx ? llwrite(NULL, 0) : llread(NULL);
    enum State state = START_STATE;

    char byte;
    //Implement the state machine

    switch (connectionParameters.role)
    {
    case LlTx:
    
        signal(SIGALRM, alarmHandler);
        while (connectionParameters.nRetransmissions > 0){

            if (alarmEnabled == FALSE) {

                alarm(connectionParameters.timeout);
                sendCommandBit(fd, A1, SET);

                alarmEnabled = TRUE;
                connectionParameters.nRetransmissions--;
            }
            
            while (state != STOP_STATE && alarmEnabled == TRUE)
            {
                switch (state)  
                {
                case START_STATE:
                    
                    readByte(&byte);

                    if (byte == FLAG)
                    {
                        state = FLAG_RCV_STATE;
                    }

                    break;
                case FLAG_RCV_STATE:
                    
                    readByte(&byte);
                    
                    switch (byte)
                    {
                    case FLAG:
                        state = FLAG_RCV_STATE;
                        break;
                    case A1:
                        state = A_RCV_STATE;
                        break;
                    default:
                        state = START_STATE;
                        break;
                    }
                    break;
                case A_RCV_STATE:
                    
                    readByte(&byte);
                    switch (byte)
                    {
                    case SET:
                        state = C_RCV_STATE;
                        break;
                    case FLAG:
                        state = FLAG_RCV_STATE;
                        break;
                    default:
                        state = START_STATE;
                        break;
                    }
                    break;
                case C_RCV_STATE:
                    
                    readByte(&byte);
                    switch (byte)
                    {
                    case A1 ^ SET:
                        state = BCC1_OK_STATE;
                        break;
                    case FLAG:
                        state = FLAG_RCV_STATE;
                        break;
                    default:
                        state = START_STATE;
                        break;
                    }
                    break;
                case BCC1_OK_STATE:
                    
                    readByte(&byte);
                    switch (byte)
                    {
                    case FLAG:
                        state = STOP_STATE;
                        break;
                    default:
                        state = START_STATE;
                        break;
                    }
                    break;
                default:
                    exit(-1);
                    break;
                }                    
            }
            
            if(state != STOP_STATE){
                exit(-1);
            }
        }
      
        break;
    case LlRx:
        
        while (state != STOP_STATE)
        {
            switch (state)
            {
            case START_STATE:
                
                readByte(&byte);
                if (byte == FLAG)
                {
                    state = FLAG_RCV_STATE;
                }
                break;
            case FLAG_RCV_STATE:
                
                readByte(&byte);
                switch (byte)
                {
                case FLAG:
                    state = FLAG_RCV_STATE;
                    break;
                case A1:
                    state = A_RCV_STATE;
                    break;
                default:
                    state = START_STATE;
                    break;
                }
                break;
            case A_RCV_STATE:
                
                readByte(&byte);
                switch (byte)
                {
                case SET:
                    state = C_RCV_STATE;
                    break;
                case FLAG:
                    state = FLAG_RCV_STATE;
                    break;
                default:
                    state = START_STATE;
                    break;
                }
                break;
            case C_RCV_STATE:
                
                readByte(&byte);
                switch (byte)
                {
                case A1 ^ SET:
                    state = BCC1_OK_STATE;
                    break;
                case FLAG:
                    state = FLAG_RCV_STATE;
                    break;
                default:
                    state = START_STATE;
                    break;
                }
                break;
            case BCC1_OK_STATE:
                
                readByte(&byte);
                switch (byte)
                {
                case FLAG:
                    state = STOP_STATE;
                    break;
                default:
                    state = START_STATE;
                    break;
                }
                break;
            default:
                exit(-1);
                break;
            }
        }

        if(state != STOP_STATE){
            exit(-1);
        }

        sendCommandBit(fd, A1, UA);

        break;
    default:
        printf("Invalid role\n");
        exit(-1);
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

int sendCommandBit(int fd , unsigned char A , unsigned char C){
    unsigned char message[5] = {FLAG, A, C, A ^ C, FLAG};
    int bytes_written = write(fd, message, 5);
    if (bytes_written != 5) {
        perror("write");
        exit(-1);
    }
    return bytes_written;
}