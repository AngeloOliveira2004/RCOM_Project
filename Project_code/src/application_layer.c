// Application layer protocol implementation

#include "application_layer.h"

#define FALSE 0
#define TRUE 1

int sequence = 0;
/*

    The application layer will fragment the data and send it one by one to the link layer

    A state machine will be implemented to handle the data transfer and error handling

*/

// Application layer main function.
// The sender/receiver must open the aplication with the following parameters: <executable_name> <serial_port_addr> <operation_mode (w/r)> <baudrate> <nRetries> <timeout> <file_name>
// Example : ./app /dev/ttyS0 w 9600 3 3 file.txt

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionParameters = {
        .serialPort = *serialPort,
        .role = strcmp(role, "r") == 0 ? LlTx : LlRx,
        .baudRate = baudRate,
        .nRetransmissions = nTries,
        .timeout = timeout,
    };

    // Open the connection
    int fd = llopen(connectionParameters);

    if (fd < 0){
        perror("Opening connection");
        exit(-1);
    }

    switch (connectionParameters.role)
    {
        case LlTx:

            FILE *file = fopen(filename, "r");
            if (file == NULL)
            {
                perror("Opening file");
                exit(-1);
            }

            int currFilePos = fseek(file, 0, SEEK_CUR);

            fseek(file, 0, SEEK_END);
            long fileSize; 
            getFilesize(file, &fileSize);
            
            long leftFileSize = fileSize - currFilePos;

            int controPacketSize = 0;
            unsigned char *controlPacket = assembleControlPacket(filename, &fileSize, 0 , &controPacketSize);

            if(llwrite(controlPacket, controPacketSize) < 0){
                perror("Sending control packet");
                exit(-1);
            }


            int dataSize = leftFileSize > T_SIZE ? T_SIZE : leftFileSize;
            long bytesLeft = fileSize - dataSize;

            while (bytesLeft > 0)
            {   
                dataSize = bytesLeft > T_SIZE ? T_SIZE : bytesLeft;
                unsigned char dataPacket = assembleDataPacket(dataSize , sequence , dataPacket);
                
                int dataSize;
                unsigned char *data = getData(file, dataSize);

                memcpy(dataPacket + 4, data, dataSize);

                if(llwrite(dataPacket, dataSize + dataPacket) < 0){
                    perror("Sending data packet");
                    exit(-1);
                }
                bytesLeft -= dataSize;
                dataSize += T_SIZE;
                sequence = (sequence + 1) % 255; //nao sei se é 99
            }
            
            unsigned char * endControlPacket = assembleControlPacket(filename, &fileSize, 3 , &controPacketSize);
            if(llwrite(endControlPacket, controPacketSize) < 0){
                perror("Sending end control packet");
                exit(-1);
            }

            llclose(0);
            
            break;
            case LlRx:
        break;
        default:
            break;
    }


    /*
    // Open the serial port
    int fd = openSerialPort(serialPort, baudRate);

    if (fd < 0)
    {
        perror("Opening serial port");
        exit(-1);
    }

    // Open the file
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        perror("Opening file");
        exit(-1);
    }

    // Read the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the file
    char *fileBuffer = malloc(fileSize);

    if (fileBuffer == NULL)
    {
        perror("Allocating memory for file");
        exit(-1);
    }

    // Read the file
    fread(fileBuffer, 1, fileSize, file);

    // Close the file
    fclose(file);

    // Send the file
    int bytesSent = 0;
    int bytesToSend = fileSize;

    while (bytesSent < bytesToSend)
    {
        // Send the data
        int bytesSentNow = llwrite(fileBuffer + bytesSent, bytesToSend - bytesSent);

        if (bytesSentNow < 0)
        {
            perror("Sending data");
            exit(-1);
        }

        // Update the number of bytes sent
        bytesSent += bytesSentNow;
    }

    // Free the memory
    free(fileBuffer);

    // Close the serial port
    closeSerialPort();
    */
}

void stuffing(char* buffer, int* size, char* stuffedBuffer){
    int i = 0;
    int j = 0;

    for(i = 0; i < *size; i++){
        if(buffer[i] == FLAG){ // buffer[i] == 0x7e
            stuffedBuffer[j] = FLAG_STUFF; // stuffedBuffer[j] = 0x7d
            j++;
            stuffedBuffer[j] = ESC_STUFF; // stuffedBuffer[j] = 0x5e
            
        }else if (buffer[i] == FLAG_STUFF){
            stuffedBuffer[j] = FLAG_STUFF; // stuffedBuffer[j] = 0x7d
            j++;
            stuffedBuffer[j] = ESC_ESC_STUFF; // stuffedBuffer[j] = 0x5d
        }
        else
        {
            stuffedBuffer[j] = buffer[i];
        }
        j++;
    }

    *size = j;
    return ; 
}

int destuffing(char* stuffedBuffer, int* size, char* destuffedBuffer){
    int i = 0;
    int j = 0;

    for(i = 0; i < *size; i++){
        if(stuffedBuffer[i] == FLAG_STUFF){ // stuffedBuffer[i] == 0x7d
            if(stuffedBuffer[i] == ESC_ESC_STUFF){ // stuffedBuffer[i+1] == 0x5d
                destuffedBuffer[j] = FLAG_STUFF; // destuffedBuffer[j] = 0x7d 
                i++;
            }else if(stuffedBuffer[i] == ESC_STUFF){ // stuffedBuffer[i+1] == 0x5e
                destuffedBuffer[j] = FLAG_STUFF; // destuffedBuffer[j] = 0x7e
                i++;
            }
        }else{
            destuffedBuffer[j] = stuffedBuffer[i];
        }
        j++;
    }
    
    *size = j;
    return 0;
}

unsigned char * assembleControlPacket(char* filename, long * filesize , int startEnd , int * controlPacketSize) {
    int filenameLen = strlen(filename);
    int bitsNecessary = (int) log2(*filesize + 1); // Calculate the number of bits necessary to store the file size
    int fileLenght = (int) ceil(bitsNecessary / 8.0); // Calculate the number of bytes necessary to store the file size and rounds up
    int packetSize = 1 + 2 + 2 + filenameLen + fileLenght; // C + TLV (file size) + TLV (file name)

    // Allocate space for the control packet
    unsigned char controlPacket[packetSize];

    int index = 0;

    // 1. Control Field (Start packet)
    controlPacket[index++] = startEnd; // Start control packet

    // 2. File Size TLV
    controlPacket[index++] = 0;       // T: file size
    controlPacket[index++] = fileLenght;       // L: 4 bytes for the integer file size
    controlPacket[index++] = (*filesize >> 24) & 0xFF; // V: file size (MSB first)
    controlPacket[index++] = (*filesize >> 16) & 0xFF;
    controlPacket[index++] = (*filesize >> 8) & 0xFF;
    controlPacket[index++] = *filesize & 0xFF;

    // 3. File Name TLV
    controlPacket[index++] = 1;                 // T: file name
    controlPacket[index++] = filenameLen & 0xFF; // L: length of the file name string
    memcpy(&controlPacket[index], filename, filenameLen); // V: file name
    index += filenameLen;

    *controlPacketSize = packetSize;
    return controlPacket; // Return the total bytes sent
}

unsigned char * assembleDataPacket(int dataSize , int sequence , unsigned char* dataPacket) {
    // Define the maximum payload size (subtracting the control, sequence, and length bytes)
    int packetSize = dataSize + 4; // Control (1) + Sequence (1) + Length (2) + Data (variable)
    int sequenceNumber = sequence;

    unsigned char dataBuffer[packetSize];

    int index = 0;
    // 1. Control Field (Data packet)
    dataPacket[index++] = 2; // Control field set to '2' for data

    // 2. Sequence Number (0-99)
    dataPacket[index++] = sequenceNumber % 100;

    // 3. Length Fields
    dataPacket[index++] = packetSize >> 8; // MSB
    dataPacket[index++] = packetSize & 0xFF; // LSB (packetSize is 2 bytes)

    // 4. Data Field (Payload)
    memcpy(&dataPacket[index], dataBuffer, packetSize); // Copy the data to the packet

    return dataPacket; 
}

unsigned char * getData(FILE * file , int dataSize){
    unsigned char * data = (unsigned char *) malloc(dataSize  * sizeof(unsigned char));
    fread(data, sizeof(unsigned char), dataSize, file);
    return data;
}

void getFilesize(FILE *file,long *filesize){
    fseek(file, 0, SEEK_END);
    *filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return ;
}
