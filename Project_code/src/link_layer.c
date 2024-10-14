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

int sequenceNumber = 99;

LinkLayerRole role;

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

    retransmitions = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
    role = connectionParameters.role;

    //connectionParameters.role == LlTx ? llwrite(NULL, 0) : llread(NULL);
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
    //BBC field will tbe the XOR of the A and C fields

    /* 
        I FRAME will tbe the frame that contains the whole data
        It will resemble this structure:

        | F | A | C | BCC1 | D1 | Data | D2 | BBC2 | F |

        before stuffing and
        after destuffing
    */

    int number_of_bytes_written = 0;
    int frameNumber = 0;
    int totalXOR = buf[0];
    
    enum ReadingState state = START_STATE;

    unsigned char * frameBuffer = (unsigned char *) malloc((bufSize + 6) * sizeof(unsigned char)); //not sure se temos de multiplicar pelo sizeof
    frameBuffer[0] = FLAG;
    frameBuffer[1] = A3;
    frameBuffer[2] = frameNumber == 0 ? C0 : C1;
    frameBuffer[3] = frameBuffer[1] ^ frameBuffer[2];


    for (size_t i = 0; i < bufSize; i++)
    {
        frameBuffer[i + 4] = buf[i];

        if(i != 0){
            totalXOR ^= buf[i];
        }
    }   

    //stuffing
    stuffing(frameBuffer, buf, &bufSize);

    //finish assembling the frame
    frameBuffer[bufSize + 1] = totalXOR;
    frameBuffer[bufSize + 2] = FLAG;

    int curRetransmitions = 0;

    signal(SIGALRM, alarmHandler);
    while (curRetransmitions < retransmitions ) {
        
        if (alarmEnabled == FALSE) {

                alarm(timeout);
                number_of_bytes_written = writeBytes((const char *)frameBuffer, bufSize + 6);

                if(number_of_bytes_written != bufSize + 6){
                    perror("write");
                    exit(-1);
                }
                
                alarmEnabled = TRUE;
                curRetransmitions++;
        }

        while(alarmEnabled == TRUE && state != STOP_STATE ){
            char byte;
            int number_of_bytes_read = readByte(&byte);
            
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
                if (byte == A3)
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
                if (byte == (A3 ^ (frameNumber == 0 ? C0 : C1)))
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
    char byte , cByte, lastByte;

    while (state != STOP_STATE)
    {
        lastByte = byte;
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
            
        case READING_STATE:

            if (byte == FLAG)
            {
                int bcc2 = packet[0];
                
                for(unsigned int i = 1;i < number_of_bytes_read;i++){
                    bcc2 ^= packet[i];
                }

                if(bcc2 == lastByte){
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
    }

    destuff(packet, &number_of_bytes_read);

    //verificar se é duplicado
    int frameNumber = C0 == cByte ? 0 : 1;

    if(sequenceNumber == packet[1]){
            memset(packet, 0, number_of_bytes_read);
            sendCommandBit(0, A1, frameNumber == 0 ? RR1 : RR0);
            return 0;
    }else{
        sequenceNumber = packet[1];
    }
    
    if (error == 0) {
        
        if(frameNumber == 0){
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
    int bytes_written = write(fd, message, 5);
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
    int j = 4;

    int extra_size = 0;
    for (unsigned int i = 0; i < *size; i++) {
        if (buffer[i] == FLAG || buffer[i] == ESCAPE_OCTET) {
            extra_size++;  
        }
    }

    frameBuffer = (unsigned char *) realloc(frameBuffer, (*size + extra_size + 1) * sizeof(unsigned char));
    if (frameBuffer == NULL) {
        // Handle allocation failure
        return;
    }

    for (unsigned int i = 0; i < *size; i++) {
        if (buffer[i] == FLAG) {
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

    for(int i = 0 ; i < *size-1; i++){
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

