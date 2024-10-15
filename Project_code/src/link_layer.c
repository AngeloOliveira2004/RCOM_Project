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
int frameNumber = 0;

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
    unsigned char * frameBuffer = (unsigned char *) malloc((bufSize + 6) * sizeof(unsigned char)); //not sure se temos de multiplicar pelo sizeof
    
    frameBuffer[0] = FLAG;
    frameBuffer[1] = A3;
    frameBuffer[2] = frameNumber == 0 ? C0 : C1;
    frameBuffer[3] = frameBuffer[1] ^ frameBuffer[2];

    for (size_t i = 0; i < bufSize; i++)
    {
        frameBuffer[i + 4] = buf[i];
    }   

    unsigned char totalXOR = buf[4];
    printf("totalXOR: 0x%02X\n", totalXOR);
    for(int i = 5; i < bufSize; i++){
        printf("buf[%d]: 0x%02X\n", i, buf[i]);
        totalXOR ^= buf[i];
    }
    printf("totalXOR: 0x%02X\n", totalXOR);
    //stuffing
    stuffing(frameBuffer, buf, &bufSize);

    //finish assembling the frame
    bufSize += 6;

    frameBuffer[bufSize - 2] = totalXOR;
    frameBuffer[bufSize - 1] = FLAG;

    printf("bufsize: %d\n", bufSize);
    FILE * file = fopen("logTransmitter.txt", "w");
    fclose(file);
    file = fopen("logTransmitter.txt", "a");
    for (size_t i = 0; i < bufSize; i++)
    {
        fprintf(file , "Byte: ");
        fprintf(file, "0x%02X \n", frameBuffer[i]);
    }
    fprintf(file, "\n");
    fclose(file);

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
        int i = 10;
        while(alarmEnabled == FALSE && state != STOP_STATE ){
            
            int number_of_bytes_read = readByte((char *) &byte);

            if(number_of_bytes_read < 1){
                continue;
            }


            if(i > 0){
                 printf("byte: 0x%02X ", byte);
                printf("State : %s\n" , getReadingStateName(state));
                i--;   
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
                if (byte == (frameNumber == 0 ? RR1 : RR0))
                {
                    state = C_RCV_STATE;
                }
                else if (byte == FLAG)
                {
                    state = FLAG_RCV_STATE;
                }
                else
                {
                    printf("byte: 0x%02X\n", byte);
                    printf("frameNumber: %d\n", frameNumber);
                    state = ERROR_STATE; 
                }
                break;
            case C_RCV_STATE:
                if (byte == (A1 ^ (frameNumber == 0 ? RR1 : RR0)))
                {
                    
                    frameNumber = frameNumber == 0 ? 1 : 0;
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

        byte = (int) byte;
        byte = byte & 0x000000FF;



        if(byte > 0xFF){
            printf("Mega Byte: 0x%02X\n", byte);
            continue;
        }
        printf("Byte: 0x%02X ", byte);
        printf("State: %s", getReadingStateName(state));
        printf("Nmbr of bytes read: %d\n", number_of_bytes_read);
        
        file = fopen("logReceiver.txt", "a");
        fprintf(file, "Byte: 0x%02X\n", byte);
        fclose(file);

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
            //printf("Byte: 0x%02X\n"  , byte);
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
               

                int bcc2 = packet[0];
                printf("BCC2: 0x%02X\n", bcc2);
                for(unsigned int i = 1;i < number_of_bytes_read - 5;i++){
                    printf("Packet[%d]: 0x%02X\n", i, packet[i]);
                    bcc2 ^= packet[i];
                }
                printf("BCC2: 0x%02X\n", bcc2);
                printf("Last byte: 0x%02X\n", lastByte);
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
        lastByte = 0xff;
        lastByte &= byte;
    }

    destuff(packet, &number_of_bytes_read);

    //verificar se é duplicado
    unsigned char frameNumber = 0;
    printf("CByte: 0x%02X\n", cByte);
    if(cByte == 0){
        frameNumber = 1;
    }else{
        frameNumber = 0;
    }
    printf("FrameNumber: %d\n", frameNumber);
    if(sequenceNumber == packet[1]){
        memset(packet, 0, number_of_bytes_read);
        sendCommandBit(0, A1, frameNumber == 0 ? RR1 : RR0);
        return 0;
    }else{
        sequenceNumber = packet[1];
    }
    
    if (error == 0) {
        printf("No error\n");
        if(frameNumber == 1){
            printf("Frame number 1\n");
            sendCommandBit(0, A1, RR1);
        }
        sendCommandBit(0, A1, RR0); 
    }
    
    if(error == 1){
        if(frameNumber == 1){
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


void stuffing(unsigned char * frameBuffer, const unsigned char* buffer, int* size){
    printf("stuffing \n");
    int j = 4;

    int extra_size = 0;
    for (unsigned int i = 4; i < *size; i++) {
        if (buffer[i] == FLAG || buffer[i] == ESCAPE_OCTET) {
            extra_size++;  
        }
    }
    printf("Extra size: %d\n", extra_size);

    frameBuffer = (unsigned char *) realloc(frameBuffer, (*size + extra_size + 1) * sizeof(unsigned char));
    if (frameBuffer == NULL) {
        printf("Error reallocating memory\n");
        return;
    }

    for (unsigned int i = 4; i < *size; i++) {
        if (buffer[i] == FLAG) {
            printf("Found FLAG\n");
            frameBuffer[j++] = ESCAPE_OCTET;       // Escape FLAG
            frameBuffer[j++] = ESCAPED_FRAME_DELIMITER;
        } else if (buffer[i] == ESCAPE_OCTET) {
            frameBuffer[j++] = ESCAPE_OCTET;       // Escape ESCAPE_OCTET
            frameBuffer[j++] = ESCAPED_ESCAPE_OCTET;
        } else {
            frameBuffer[j++] = buffer[i];         // Copy byte
        }
    }

    *size = j;
    return ; 
}


void destuff(unsigned char* stuffedBuffer, int* size){

    unsigned char* deStuffedBuffer = malloc(*size * sizeof(unsigned char));

    int actualSize = 0;

    for(int i = 4 ; i < *size-1; i++){
        if(stuffedBuffer[i] == ESCAPE_OCTET){
            if(stuffedBuffer[i+1] == ESCAPED_ESCAPE_OCTET){
                deStuffedBuffer[actualSize++] = ESCAPE_OCTET;
                i++;
            }else if(stuffedBuffer[i+1] == ESCAPED_FRAME_DELIMITER){
                deStuffedBuffer[actualSize++] = FLAG;
                i++;
            }
        }
    }

    *size = actualSize;
    stuffedBuffer = memcpy(stuffedBuffer, deStuffedBuffer, actualSize);
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
