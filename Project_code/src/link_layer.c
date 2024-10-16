// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "globals.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = FALSE;
int alarmCount = 0;
int retransmitions = 0;
int timeout = 0;
int frameNumberWrite = 0;
int frameNumberRead = 0;

int sequenceNumber = 99;
int curRetransmissions = 3;

LinkLayerRole role;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

void alarmHandler(int signal)
{
    alarmEnabled = TRUE;
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

    retransmitions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
    role = connectionParameters.role;

    enum ReadingState state = START_STATE;
    char byte = 0x00;


    switch (connectionParameters.role)
    {
    case LlTx:

        signal(SIGALRM, alarmHandler);
        while (connectionParameters.nRetransmissions > 0 && state != STOP_STATE){
            
            sendCommandBit(fd, A3, SET);
            alarm(connectionParameters.timeout);
            alarmEnabled = FALSE;
            
            while (state != STOP_STATE && alarmEnabled == FALSE)
            {   
                if(readByte(&byte) < 1){
                    continue;
                }

                switch (state)  
                {
                case START_STATE:
                    if (byte == FLAG)
                    {
                        state = FLAG_RCV_STATE;
                    }

                    break;
                case FLAG_RCV_STATE:
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
                    switch (byte)
                    {
                    case UA:
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
                    
                   
                    switch (byte)
                    {
                    case A1 ^ UA:
                        state = BCC_OK_STATE;
                        break;
                    case FLAG:
                        state = FLAG_RCV_STATE;
                        break;
                    default:
                        state = START_STATE;
                        break;
                    }
                    break;
                case BCC_OK_STATE:
                    
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
            connectionParameters.nRetransmissions--;
        }

        if(state != STOP_STATE){
            perror("Connection failed");
            exit(-1);
        }
      
        break;
    case LlRx:

        while (state != STOP_STATE)
        {   
            if(readByte(&byte) < 1){
                continue;
            }
            
            switch (state)
            {
            case START_STATE:
                if (byte == FLAG)
                {
                    state = FLAG_RCV_STATE;
                }
                break;
            case FLAG_RCV_STATE:
                switch (byte)
                {
                case FLAG:
                    state = FLAG_RCV_STATE;
                    break;
                case A3:
                    state = A_RCV_STATE;
                    break;
                default:
                    state = START_STATE;
                    break;
                }
                break;
            case A_RCV_STATE:
                
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
                
                switch (byte)
                {
                case A3 ^ SET:
                    state = BCC_OK_STATE;
                    break;
                case FLAG:
                    state = FLAG_RCV_STATE;
                    break;
                default:
                    state = START_STATE;
                    break;
                }
                break;
            case BCC_OK_STATE:
                
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

    printf("Connection established\n");
    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int number_of_bytes_written = 0;

    alarmEnabled = FALSE;
    alarmCount = 0;
    
    enum ReadingState state = START_STATE;
    unsigned char * frameBuffer = malloc((2*bufSize + 7) * sizeof(unsigned char)); //not sure se temos de multiplicar pelo sizeof
    
    frameBuffer[0] = FLAG;
    frameBuffer[1] = A3;
    frameBuffer[2] = frameNumberWrite == 0 ? C0 : C1;
    frameBuffer[3] = frameBuffer[1] ^ frameBuffer[2];

    unsigned char totalXOR = buf[0]; 
    for(int i = 1; i < bufSize; i++){
        totalXOR ^= buf[i];
    }

    for (size_t i = 0; i < bufSize; i++)
    {
        frameBuffer[i + 4] = buf[i];
    }

    frameBuffer[bufSize + 4] = totalXOR;
    bufSize += 1;
    
    unsigned char * completeBuffer = malloc((2*bufSize + 7) * sizeof(unsigned char));

    bufSize += 4;

    frameBuffer[bufSize] = totalXOR;
    
    completeBuffer = stuffing(frameBuffer, &bufSize);

    bufSize += 1;
    completeBuffer[bufSize - 1] = FLAG;

    int curRetransmitions = retransmitions;

    unsigned char byte = 0x00;

    signal(SIGALRM, alarmHandler);
    while (curRetransmitions > 0 && state != STOP_STATE) {
        

        number_of_bytes_written = writeBytes((const char *)frameBuffer, bufSize);
        printf("Number of bytes written: %d\n", number_of_bytes_written);

        if(number_of_bytes_written != bufSize){
                    printf("Error writing to serial port\n");
                    perror("write");
                    exit(-1);
        }

        alarm(timeout);
        alarmEnabled = FALSE;

        state = START_STATE;

        while(alarmEnabled == FALSE && state != STOP_STATE ){
            
            int number_of_bytes_read = readByte((char *) &byte);

            if(number_of_bytes_read < 1){
                continue;
            }

            switch (state) {
            case START_STATE:
                if (byte == FLAG)
                {
                    state = FLAG_RCV_STATE;
                }
                break;
            case FLAG_RCV_STATE:
                if (byte == A1 || byte == RR1)
                {
                    state = A_RCV_STATE;
                }
                else if (byte == FLAG)
                {
                    state = FLAG_RCV_STATE;
                }
                else
                {
                    state = ERROR_STATE; 

                }
                break;
            case A_RCV_STATE:
                if (byte == (frameNumberWrite == 0 ? RR1 : RR0))
                {
                    state = C_RCV_STATE;
                }
                else if (byte == FLAG)
                {
                    state = FLAG_RCV_STATE;
                }
                else
                {
                    state = ERROR_STATE; 
                }
                break;
            case C_RCV_STATE:
                if (byte == (A1 ^ (frameNumberWrite == 0 ? RR1 : RR0)))
                {
                    frameNumberWrite = frameNumberWrite == 0 ? 1 : 0;
                    state = BCC_OK_STATE;
                }
                else
                {
                    state = START_STATE;
                }
                break;
            case BCC_OK_STATE:
                if (byte == FLAG)
                {
                    state = STOP_STATE;
                }
                else
                {
                    state = START_STATE;
                }
                break;
            case REJ0_STATE:
                alarmEnabled = FALSE;
                frameNumberWrite = frameNumberWrite == 0 ? 1 : 0;
                state = STOP_STATE;
            
                break;
            case REJ1_STATE:
                alarmEnabled = FALSE;
                frameNumberWrite = frameNumberWrite == 0 ? 1 : 0;
                state = STOP_STATE;
                break;
            case ERROR_STATE:
                alarmEnabled = FALSE;
                break;
            default:
                break;
            }
        }

        if(state == STOP_STATE){
            return number_of_bytes_written;
        }

        curRetransmitions--;
    }
    
    printf("Error sending frame\n");
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{      
    int number_of_bytes_read = 0;
    int error = 0;

    enum ReadingState state = START_STATE;
    unsigned char byte = 0x00 , cByte = 0x00 , lastByte = 0x00;

    FILE * file = fopen("logReceiver.txt", "w");
    fclose(file);

    printf("Reading\n");
    while (state != STOP_STATE)
    {
        
        if(readByte((char *) &byte) < 1){
            continue;
        }

        printf("Byte: 0x%02X ", byte);
        printf("State: %s\n", getReadingStateName(state));
/*
        printf("Byte: 0x%02X ", byte);
        printf("State: %s", getReadingStateName(state));
        printf("Nmbr of bytes read: %d\n", number_of_bytes_read);
        
        file = fopen("logReceiver.txt", "a");
        fprintf(file, "Byte: 0x%02X\n", byte);
        fclose(file);
*/
        switch (state)
        {
        case START_STATE:
            if (byte == FLAG)
            {
                state = FLAG_RCV_STATE;
            }
            break;
        case FLAG_RCV_STATE:
            if (byte == A3) {
                state = A_RCV_STATE;
            } else if (byte == FLAG) {
                state = FLAG_RCV_STATE;
            } else if (byte != FLAG) {
                state = ERROR_STATE; 
                error = 1;
            }
            break;
        case A_RCV_STATE:

            if(byte == C0 || byte == C1){
                cByte = byte;
                state = C_RCV_STATE;
            } else if (byte == FLAG) {
                state = FLAG_RCV_STATE;
            } else {
                state = ERROR_STATE;
                error = 1;
            }
            break;
        case C_RCV_STATE:
            if(byte == (A3 ^ cByte)){
                state = READING_STATE;
            } else if (byte == FLAG){
                state = FLAG_RCV_STATE;
            } else {
                error = 1;
                state = ERROR_STATE;
            }
            break;
        case READING_STATE:
            if (byte == FLAG)
            {
                printf("Found flag\n");

                printf("Before Destuffing\n");
                for (size_t i = 0; i < number_of_bytes_read; i++)
                {
                    printf("Packet[%ld]: 0x%02X\n", i, packet[i]);
                }
                
                destuff(packet, &number_of_bytes_read);

                printf("After Destuffing\n");
                for (size_t i = 0; i < number_of_bytes_read; i++)
                {
                    printf("Packet[%ld]: 0x%02X\n", i, packet[i]);
                }

                int bcc2 = packet[0];
                
                printf("LAST BYTE: 0x%02X\n", lastByte);

                for(unsigned int i = 1;i < number_of_bytes_read-1; i++){
                    printf("Packet[%d]: 0x%02X\n", i, packet[i]);
                    bcc2 ^= packet[i];
                }

                printf("Lats byte: 0x%02X\n", bcc2);

                if(lastByte == bcc2){
                    state = STOP_STATE;
                }else{
                    packet[number_of_bytes_read++] = byte;
                }

            }else {
                packet[number_of_bytes_read++] = byte;
            }
            break;
        default:
            break;
        }
        lastByte = byte;
    }

    if(packet[0] != 2){
        if (error == 0) {
            if(frameNumberRead == 1){
                frameNumberRead = 0;
                printf("Frame number 1\n");
                sendCommandBit(0, A1, RR0);
            }else{
                frameNumberRead = 1;
                sendCommandBit(0, A1, RR1); 
            }
        }
    
        if(error == 1){
            if(frameNumberRead == 1){
                sendCommandBit(0, A1, REJ1);
            }else{
                sendCommandBit(0, A1, REJ0);
            }
        }
    }
    
    if(sequenceNumber == packet[1]){
        memset(packet, 0, number_of_bytes_read);
        sendCommandBit(0, A1, frameNumberRead == 0 ? RR1 : RR0);
        return 0;
    }else{
        sequenceNumber = packet[1];
    }
    
    if (error == 0) {
        if(frameNumberRead == 1){
            frameNumberRead = 0;
            sendCommandBit(0, A1, RR0);
        }
        sendCommandBit(0, A1, RR1); 
    }
    
    if(error == 1){
        if(frameNumberRead == 1){
            sendCommandBit(0, A1, REJ1);
        }
        sendCommandBit(0, A1, REJ0);
    }
    
    return number_of_bytes_read;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){
    enum ReadingState state = START_STATE;

    int curRetransmitions = 0;

    char byte;

    switch (role) {
    case LlTx:
    
        signal(SIGALRM, alarmHandler);
        while (retransmitions > curRetransmitions){

            if (alarmEnabled == FALSE) {

                alarm(timeout);
                sendCommandBit(1, A3, DISC);

                alarmEnabled = TRUE;
                curRetransmitions++;
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
                    case DISC:
                        state = DISC_RCV_STATE;
                        break;
                    case FLAG:
                        state = FLAG_RCV_STATE;
                        break;
                    default:
                        state = START_STATE;
                        break;
                    }
                    break;
                case DISC_RCV_STATE:
                    
                    readByte(&byte);
                    switch (byte)
                    {
                    case A1 ^ DISC:
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

        sendCommandBit(1, A3, UA);
      
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
                case A3:
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
                case DISC:
                    state = DISC_RCV_STATE;
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
                case A3 ^ DISC:
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

        sendCommandBit(0, A3, UA);

        break;
    default:
        printf("Invalid role\n");
        exit(-1);
    }

    int clstat = closeSerialPort();
    return clstat;
}

int sendCommandBit(int fd , unsigned char A , unsigned char C){
    unsigned char message[5] = {FLAG, A, C, A ^ C, FLAG};
     
    int bytes_written = writeBytes((const char *)message, 5);

    if (bytes_written != 5) {
        perror("write");
        exit(-1);
    }          
    return bytes_written;
}


FrameType parsePacketHeader(const unsigned char *packet) {

    unsigned char controlByte = packet[2];
    unsigned char bcc = packet[3];

    // Check BCC (Byte Control Check)
    if (bcc != (A1 ^ controlByte)) {
        return Error_Frame;
    }

    // Determine frame type based on the control byte
    switch (controlByte) {
        case SET:
        case UA:
            return S_Frame;
            break;
        case DISC:
            return Disc_Frame;
            break;
        case RR0:
            return RR0_Frame;
            break;
        case RR1:
            return RR1_Frame;
            break;
        case REJ0:
            return REJ0_Frame;
            break;
        case REJ1:
            return REJ1_Frame;
            break;
        default:
            return Error_Frame; 
        }
}


unsigned char * stuffing(unsigned char *frameBuffer, int* size) {
    printf("Stuffing\n");

    int j = 4;
    int extra_size = 0;

    for (int i = 4; i < *size; i++) {
        printf("FrameBuffer[%d]: 0x%02X\n", i, frameBuffer[i]);
        if (frameBuffer[i] == FLAG || frameBuffer[i] == ESCAPE_OCTET) {
            extra_size++;
        }
    }

    printf("Extra size: %d\n", extra_size);
    
    if(extra_size == 0){
        return frameBuffer;
    }

    unsigned char * completeBuffer = (unsigned char *) malloc((*size + extra_size) * sizeof(unsigned char));

    memcpy(completeBuffer, frameBuffer, 4);

    for (int i = 4; i < *size; i++) {
        if (frameBuffer[i] == FLAG) {
            completeBuffer[j++] = ESCAPE_OCTET;
            completeBuffer[j++] = ESCAPED_FRAME_DELIMITER;
        } else if (frameBuffer[i] == ESCAPE_OCTET) {
            completeBuffer[j++] = ESCAPE_OCTET;
            completeBuffer[j++] = ESCAPED_ESCAPE_OCTET;
        } else {
            completeBuffer[j++] = frameBuffer[i];
        }
    }

    *size += extra_size;    
    
    return completeBuffer;
}



void destuff(unsigned char* stuffedBuffer, int* size){

    unsigned char* deStuffedBuffer = malloc(*size * sizeof(unsigned char));

    int actualSize = 0;

    for(int i = 0 ; i < *size; i++){
        if(stuffedBuffer[i] == ESCAPE_OCTET){
            printf("Found escape octet\n");
            if(stuffedBuffer[i+1] == ESCAPED_ESCAPE_OCTET){
                deStuffedBuffer[actualSize++] = ESCAPE_OCTET;
                i++;
            }else if(stuffedBuffer[i+1] == ESCAPED_FRAME_DELIMITER){
                deStuffedBuffer[actualSize++] = FLAG;
                i++;
            }
        }
        else{
            deStuffedBuffer[actualSize++] = stuffedBuffer[i];
        }
    }
    
    
    if(actualSize == *size){
        free(deStuffedBuffer);
        return;
    }

    printf("Actual size: %d\n", actualSize);
    printf("Size: %d\n", *size);

    *size = actualSize;
    
    printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA ERROR IN REALOC \n");
    stuffedBuffer = realloc(stuffedBuffer, actualSize);

    return ;
}


char* getReadingStateName(enum ReadingState state) {
    switch (state) {
        case START_STATE:      return "START_STATE";
        case FLAG_RCV_STATE:   return "FLAG_RCV_STATE";
        case A_RCV_STATE:      return "A_RCV_STATE";
        case C_RCV_STATE:      return "C_RCV_STATE";
        case BCC_OK_STATE:     return "BCC_OK_STATE";
        case DATA_RCV_STATE:   return "DATA_RCV_STATE";
        case FOUND_ESC_STATE:  return "FOUND_ESC_STATE";
        case DISCONNECT_STATE: return "DISCONNECT_STATE";
        case READING_STATE:    return "READING_STATE";
        case STOP_STATE:       return "STOP_STATE";
        case BCC1_OK_STATE:    return "BCC1_OK_STATE";
        case ERROR_STATE:      return "ERROR_STATE";
        case DISC_RCV_STATE:   return "DISC_RCV_STATE";
        default:               return "UNKNOWN_STATE";
    }
}
