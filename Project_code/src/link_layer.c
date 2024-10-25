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
clock_t start,end;

int sequenceNumber = 50;

Statistics statistics;
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


void disAlarm(){
    char rbyte = 0x00;

    while(1){
        int number_of_bytes_read = readByte(&rbyte);

        if(number_of_bytes_read < 1){
            break;
        }
    }

    alarmEnabled = TRUE;
    alarmCount = 0;
    alarm(0);
}


int llopen(LinkLayer connectionParameters)
{
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate); // Open the serial port with the parameters defined in the struct

    if (fd < 0){
        return -1;
    }

    start = clock();

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
            statistics.numSUFrames++;
            sendCommandBit(fd, A3, SET);
            alarm(connectionParameters.timeout);
            alarmEnabled = FALSE;
            
            while (state != STOP_STATE && alarmEnabled == FALSE)
            {   
                if(readByte((char *) &byte) < 1){
                    continue;
                }

                transitionNextState(&byte, &state, A1, UA);                   
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
            if(readByte((char *) &byte) < 1){
                continue;
            }
            
            transitionNextState(&byte, &state, A3, SET);
        }

        if(state == STOP_STATE){
            statistics.numSUFrames++;
            sendCommandBit(fd, A1, UA);
        }

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

    int originalSize = bufSize;
    
    enum ReadingState state = START_STATE;
    unsigned char * frameBuffer = malloc((2*bufSize + 7) * sizeof(unsigned char));
    
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

    FILE * file = fopen("logTransmitter.txt", "a");
    fprintf(file, "Sent frame: ");
    for(int i = 4; i < bufSize-2; i++){
        fprintf(file, "%x ", completeBuffer[i]);
    }
    //fprintf(file, "%d", bufSize);
    fprintf(file, "\n");
    fclose(file);


    bufSize += 1;
    completeBuffer[bufSize - 1] = FLAG;


    int curRetransmitions = retransmitions;

    unsigned char byte = 0x00;

    signal(SIGALRM, alarmHandler);
    while (alarmCount < curRetransmitions && state != STOP_STATE) {
        statistics.numIFrames++;
        
        number_of_bytes_written = writeBytes((const char *)completeBuffer, bufSize);

        printf("Number of bytes written: %d\n", number_of_bytes_written);

        if(number_of_bytes_written != bufSize){
                    printf("Error writing to serial port\n");
                    perror("write");
                    exit(-1);
        }

        alarmEnabled = FALSE;
        alarm(timeout);

        state = START_STATE;

        while(alarmEnabled == FALSE && state != STOP_STATE ){
            
            int number_of_bytes_read = readByte((char *) &byte);

            if(number_of_bytes_read < 1){
                continue;
            }

            switch (state) {
            case START_STATE:
                if (byte == FLAG){
                    state = FLAG_RCV_STATE;
                }
                break;
            case FLAG_RCV_STATE:
                if (byte == A1){
                    state = A_RCV_STATE;
                }
                else if (byte == FLAG){
                    state = FLAG_RCV_STATE;
                }
                else{
                    state = ERROR_STATE;
                }
                break;
            case A_RCV_STATE:
                //recebeu o AB a pedir o frame 0 logo frameNumberWrite tem de ser igual a 0
                if (byte == (frameNumberWrite == 0 ? RR1 : RR0)){
                    state = C_RCV_STATE;
                }
                else if (byte == FLAG){
                    state = FLAG_RCV_STATE;
                }
                else if(byte == REJ0 || byte == REJ1){
                    state = REJ0_STATE;
                }

                else{
                    state = ERROR_STATE; 
                }
                break;
            case C_RCV_STATE:
                if (byte == (A1 ^ (frameNumberWrite == 0 ? RR1 : RR0))){
                    state = BCC_OK_STATE;
                }
                else{
                    state = START_STATE;
                }
                break;
            case BCC_OK_STATE:
                if (byte == FLAG){
                    frameNumberWrite = frameNumberWrite == 0 ? 1 : 0;
                    state = STOP_STATE;
                }
                else{
                    state = START_STATE;
                }
                break;
            case REJ0_STATE:
            case REJ1_STATE:
                printf("REJ received\n");
                statistics.numREJ++;
                disAlarm();
                state = START_STATE;
                curRetransmitions++; // Incrementa o numero de retransmissoes porque se receber o rej1 não conta como uma falha de transmissão
                break;
            case ERROR_STATE:
                alarmEnabled = FALSE;
                break;
            default:
                break;
            }
        }


        if(state == STOP_STATE){
            statistics.numSuccessFrames++;
            free(frameBuffer);
            return originalSize;
        }

        statistics.numTimeouts++;
        curRetransmitions--;
    }
    
    free(frameBuffer);
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
    int reading = 0;

    enum ReadingState state = START_STATE;
    unsigned char byte = 0x00 , cByte = 0x00 ;

    statistics.numIFrames++;

    while (state != STOP_STATE || reading == 0)
    {
        
        if(readByte((char *) &byte) < 1){
            continue;
        }

        //printf("Byte: %x\n", byte);
        //printf("State: %s\n", getReadingStateName(state));
        
        if(state == STOP_STATE){
            break;
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
                reading = 1;
                state = C_RCV_STATE;
            } else if (byte == FLAG) {
                state = FLAG_RCV_STATE;
            } else if(byte == SET){
                state = HEADER_ERROR_STATE;
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
        case ERROR_STATE:
            error = 1;
            state = STOP_STATE;
            break;
        case READING_STATE: {
            if (byte == FLAG)
            {
                number_of_bytes_read = destuff(packet, number_of_bytes_read);
                
                int bcc2 = packet[0];
                for (unsigned int i = 1; i < number_of_bytes_read - 1; i++) {
                    bcc2 ^= packet[i];
                }
                
                if (packet[number_of_bytes_read - 1] == bcc2) {
                    number_of_bytes_read -= 1;
                    state = STOP_STATE;
                    
                } else {
                    memset(packet, 0, number_of_bytes_read);
                    number_of_bytes_read = 0;
                    error = 1;
                    reading = 1;
                    state = STOP_STATE;
                }

            }else {
                packet[number_of_bytes_read++] = byte;
            }
            break;
        }
        case HEADER_ERROR_STATE:
            reading = 0;
            if(byte == (SET ^ A3)){
                sendCommandBit(0, A3, UA);
            }
            state = START_STATE;
            break;
        default:
            break;
        }
        
    }  
    
            
    FILE * file = fopen("logDup.txt", "a");
    fprintf(file, "Cur seq numer : %x", packet[1]);
    fprintf(file, " Last seq numer : %i", sequenceNumber);
    fprintf(file, "Error: %d\n", error);
    fprintf(file, "\n");
    fclose(file);


    if(packet[0] == 2 && error == 0){

        int packetSentSize = packet[2] << 8 | packet[3];

        if(packetSentSize != number_of_bytes_read - 4){
            printf("Error reading frame\n");
            printf("packetSentSize %i" , packetSentSize);
            printf("number_of_bytes_read %i" , number_of_bytes_read);
        }


        if(sequenceNumber == (int) packet[1]){
            printf("Repeated frame\n");
            //printf("Last packet size: %i\n", lastPacketSize);
            printf("Number of bytes read: %i\n", number_of_bytes_read);
            error = 0;
            statistics.numSuccessFrames--;
            memset(packet, 0, number_of_bytes_read);
            number_of_bytes_read = 0;
        }else{
            sequenceNumber = (int) packet[1];
            //lastPacketSize = number_of_bytes_read;
        }
    }

    if (error == 0) {
        statistics.numSuccessFrames++;
        if(frameNumberRead == 1){
            frameNumberRead = 0;
            sendCommandBit(0, A1, RR0);
        }else{
            frameNumberRead = 1;
            sendCommandBit(0, A1, RR1);
        }
    }
    
    if(error == 1){
        memset(packet, 0, number_of_bytes_read);
        number_of_bytes_read = 0;
        statistics.numREJ++;
        if(frameNumberRead == 1){
            sendCommandBit(0, A1, REJ1);
        }else{
            sendCommandBit(0, A1, REJ0);
        }
    }
    

    return number_of_bytes_read;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics){

    enum ReadingState state = START_STATE;

    int curRetransmitions = retransmitions;

    char byte;

    switch (role) {
    case LlTx:
        
        signal(SIGALRM, alarmHandler);
        while (curRetransmitions > 0 && state != STOP_STATE){
            
            sendCommandBit(1, A3, DISC);
            statistics.numSUFrames++;
            alarm(timeout);
            alarmEnabled = FALSE;
            
            while (state != STOP_STATE && alarmEnabled == FALSE)
            {
                
                if(readByte((char *)&byte) < 1){
                    continue;
                }

                transitionNextState(&byte, &state, A1, DISC);                 
            }
            
            if(state != STOP_STATE){
                exit(-1);
            }
        }

        sendCommandBit(1, A3, UA);
        statistics.numSUFrames++;
      
        break;
    case LlRx:
        
        while (state != STOP_STATE)
        {
            if(readByte((char *)&byte) < 1){
                    continue;
            }

            printf("Byte: %x\n", byte);
            printf("State: %s\n", getReadingStateName(state));

            transitionNextState(&byte, &state, A3, DISC);
        }

        sendCommandBit(1, A1, DISC);
        statistics.numSUFrames++;

        state = START_STATE;

        while (state != STOP_STATE)
        {
            if(readByte((char *)&byte) < 1){
                    continue;
            }

            transitionNextState(&byte, &state, A3, UA);
        }

        if(state != STOP_STATE){
            
            exit(-1);
        }

        break;
    default:
        printf("Invalid role\n");
        exit(-1);
    }

    printf("Connection closed\n");

    end = clock();
    float elapsed = (float) (end - start) / CLOCKS_PER_SEC;

    if(showStatistics){
        printf("Statistics:\n");
        printf("  - Number of S and U frames: %d\n", statistics.numSUFrames);
        printf("  - Number of I frames: %d\n", statistics.numIFrames);
        printf("  - Number of successful I frames: %d\n", statistics.numSuccessFrames);
        printf("  - Number of REJ frames: %d\n", statistics.numREJ);
        printf("  - Number of timeouts: %d\n", statistics.numTimeouts);
        printf("  - Elapsed time: %.2f seconds\n", elapsed);
    }

    int clstat = closeSerialPort();
    return clstat;
}

int sendCommandBit(int fd , unsigned char A , unsigned char C){
    unsigned char message[5] = {FLAG, A, C, A ^ C, FLAG};
     
    int bytes_written = writeBytes((const char *)message, 5);

    if (bytes_written != 5) {
        printf("Error writing to serial port\n");
        perror("write");
        exit(-1);
    }   
           
    return bytes_written;
}


unsigned char * stuffing(unsigned char *frameBuffer, int* size) {

    int j = 4;
    int extra_size = 0;

    for (int i = 4; i < *size; i++) {
        if (frameBuffer[i] == FLAG || frameBuffer[i] == ESCAPE_OCTET) {
            extra_size++;
        }
    }
    
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



int destuff(unsigned char* stuffedBuffer, int size){

    unsigned char* deStuffedBuffer = (unsigned char *)  malloc(2*size * sizeof(unsigned char));

    int actualSize = 0;

    for(int i = 0 ; i < size; i++){
        if(stuffedBuffer[i] == ESCAPE_OCTET){
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
    
    if(actualSize == size){
        free(deStuffedBuffer);
        return size;
    }

    memcpy(stuffedBuffer, deStuffedBuffer, actualSize);

    free(deStuffedBuffer);

    return actualSize;
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
        case ERROR_STATE:      return "ERROR_STATE";
        case DISC_RCV_STATE:   return "DISC_RCV_STATE";
        default:               return "UNKNOWN_STATE";
    }
}


int transitionNextState(char *byte, enum ReadingState *state, int A, int C) {
    switch (*state) {
        case START_STATE:
            if (*byte == FLAG) *state = FLAG_RCV_STATE;
            break;

        case FLAG_RCV_STATE:

            if (*byte == A) *state = A_RCV_STATE;
            else if (*byte == FLAG) *state = FLAG_RCV_STATE;
            else *state = START_STATE;
            break;

        case A_RCV_STATE:

            if (*byte == C) *state = C_RCV_STATE;
            else if(*byte == FLAG) *state = FLAG_RCV_STATE;
            else *state = START_STATE;
            break;

        case C_RCV_STATE:

            if (*byte == (A ^ C)) *state = BCC_OK_STATE;
            else if(*byte == FLAG) *state = FLAG_RCV_STATE;
            else *state = START_STATE;
            break;

        case BCC_OK_STATE:

            if (*byte == FLAG) *state = STOP_STATE;
            else *state = START_STATE;
            break;

        default:
            break;
    }
    return 0;
}