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
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate); // Open the serial port with the parameters defined in the struct

    if (fd < 0)
    {
        return -1;
    }

    connectionParameters.role == LlTx ? llwrite(NULL, 0) : llread(NULL);
    enum ReadingState state = START_STATE;

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
    int error = 0;
    enum WritingState state = START_WRITING_STATE;

    unsigned char * packet = (unsigned char *)  malloc(bufSize + 5);
    packet[0] = FLAG;
    packet[1] = A1;
    packet[2] = SET;
    packet[3] = A1 ^ SET;
    packet[4] = FLAG;

    memcpy(packet + 5, buf, bufSize);

    unsigned char byte = buf[0];

    for (size_t i = 0; i < bufSize; i++)
    {
        packet[i + 5] = buf[i] ^ byte;
    }
    

    while (state != STOP_WRITING_STATE)
    {
        switch (state)
        {
        case START_WRITING_STATE:
            sendCommandBit(0, A1, SET);
            break;
        
        default:
            break;
        }
    }
    


    return number_of_bytes_written;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{   

    /*
        Frames will have a header , body and a trailer
        
        Rvery time a frame is received without errors and in the right sequence a positive
        acknowledgement is sent back to the sender.


        Use of timers (time-out) to enable re-transmission of un-acknowledged frames
        Use of negative acknowledgement to request the retransmission of out-of-sequence or
        errored frames
        Verification of duplicates which may occur due to re-transmissions

        A frame can be started with one or more flags, which must be taken into account
        by the frame reception mechanism
     */
    
    int number_of_bytes_read = 0;
    int error = 0;
    int isDuplicate = 0; // New variable to track duplicate frames
    int dataError = 0;   // New variable to track data errors
    static unsigned char lastSeqNum = -1;  // Store last sequence number

    enum ReadingState state = START_STATE;
    char byte , cByte , seqNum;

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
            } else if (byte == FLAG) {
                state = FLAG_RCV_STATE;
            } else if (byte != FLAG) {
                state = ERROR_STATE; // Remain in the same state if incorrect byte
                error = 1;
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
                error = 1;
                state = ERROR_STATE;
                break;
            }
            break;
        case C_RCV_STATE:
            seqNum = byte; // Store sequence number
            if (seqNum == lastSeqNum) {
                isDuplicate = 1; // Mark as duplicate
            } else {
                lastSeqNum = seqNum; // Update last sequence number
            }

            if(byte == (A1 ^ cByte)){
                state = READING_STATE;
            } else if (byte == FLAG){
                state = FLAG_RCV_STATE;
            } else {
                error = 1;
                state = ERROR_STATE;
            }
            
        case READING_STATE:
            //TODO: Implement the reading of the data
            //Dont know what to do to terminate the reading

            //check if the nuber of 1s is even

            //if even continue reading otherwise go to error state

            if(countZerosFromPacket(packet, number_of_bytes_read) == 0){
                error = 1;
                state = ERROR_STATE;
                break;
            }

            if (byte == FLAG)
            {
                state = STOP_STATE;
            }
            else if (byte == ESC_STUFF)
            {
                state = FOUND_ESC_STATE;
            } else if (byte == (A1 ^ SET)){
                state = BCC1_OK_STATE;
            }
            else
            {
                packet[number_of_bytes_read] = byte;
                number_of_bytes_read++;
            }
            break;
        case BCC1_OK_STATE:
            if (byte == FLAG)
            {
                state = STOP_STATE;
            }
            else
            {
                state = START_STATE;
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
        case ERROR_STATE:
            if (dataError) {
                    sendCommandBit(0, A1, REJ0); // Send REJ if data error
            }
            return -1;
            break;
        case DISCONNECT_STATE:
            sendCommandBit(0, A1, DISC);
            return 0;
        default:
            break;
        }
    }

    
    if (isDuplicate) {
        sendCommandBit(0, A1, REJ0); // Confirm duplicate with RR
    } else if (!error && !dataError) {
        sendCommandBit(0, A1, REJ0); // Send RR if no errors
    }

    if(error == 1){
        return -1;
    }
    
    return number_of_bytes_read;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{

    enum ControlStates state = START;
    char byte;
    

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

int countZerosFromPacket(unsigned char *packet, int packetSize){
    int count = 0;

    if(packetSize == 0){
        return 1;
    }

    for(int i = 0 ; i < packetSize ; i++){
        char packetChar = packet[i];
        while (packetChar != 0)
        {
            count += packetChar & 1;
            packetChar >>= 1;
        }
    }
    return count % 2 == 0;
}