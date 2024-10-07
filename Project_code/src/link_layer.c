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

    
    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int number_of_bytes_written = 0;

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int number_of_bytes_read = 0;

    enum ReadingState state = START_STATE;
    char byte , cByte;

    while (state != STOP_STATE)
    {
        readByte(&byte);
        switch (state)
        {
        case START_STATE:
            if (byte == FLAG)
            {
                state = FLAG_RCV_STATE;
            }
            break;
        case FLAG_RCV_STATE:
            if (byte == A1) {
                state = A_RCV_STATE;
            } else if (byte != FLAG) {
                state = START_STATE; // Remain in the same state if incorrect byte
            }
            break;
        case A_RCV_STATE:

            switch (byte)
            {
            case SET:
                state = C_RCV_STATE;
                break;
            case DISC:
                state = DISCONNECT_STATE;
                break;
            case FLAG:
                state = FLAG_RCV_STATE;
                break;
            case RR0:
                state = C_RCV_STATE;
                cByte = RR0;
                break;
            case RR1:
                state = C_RCV_STATE;
                cByte = RR1;
                break;
            default:
                state = START_STATE;
                break;
            }
            break;
        case C_RCV_STATE:

            if(byte == (A1 ^ cByte)){
                state = READING_STATE;
            } else if (byte == FLAG){
                state = FLAG_RCV_STATE;
            } else {
                state = START_STATE;
            }
        case READING_STATE:
            //TODO: Implement the reading of the data
            //Dont know what to do to terminate the reading
            if (byte == FLAG)
            {
                state = STOP_STATE;
            }
            else if (byte == ESC_STUFF)
            {
                state = FOUND_ESC_STATE;
            }
            else
            {
                packet[number_of_bytes_read] = byte;
                number_of_bytes_read++;
            }
            break;
        case FOUND_ESC_STATE:
            state = READING_STATE;
            if (byte == FLAG)
            {
                packet[number_of_bytes_read] = FLAG_STUFF;
                number_of_bytes_read++;
                packet[number_of_bytes_read] = ESC_STUFF;
                number_of_bytes_read++;   
            }
            else if (byte == FLAG_STUFF)
            {
                packet[number_of_bytes_read] = FLAG_STUFF;
                number_of_bytes_read++;
                packet[number_of_bytes_read] = ESC_ESC_STUFF;
                number_of_bytes_read++;
            }
            else
            {
                packet[number_of_bytes_read] = byte;
                number_of_bytes_read++;
            }
            break;
        case DISCONNECT_STATE:
            sendCommandBit(0, A1, DISC);
            return 0;
        default:
            break;
        }
    }
    

    return number_of_bytes_read;
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