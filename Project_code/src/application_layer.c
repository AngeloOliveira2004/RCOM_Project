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

        .role = strcmp(role, "rx") == 0 ? LlRx : LlTx,
        .baudRate = baudRate,
        .nRetransmissions = nTries,
        .timeout = timeout
    };

    strcpy(connectionParameters.serialPort, serialPort);

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

            long fileSize; 
            getFilesize(file, &fileSize);
            
            long leftFileSize = fileSize - currFilePos;

            int controPacketSize = 0;

            unsigned char *controlPacket = assembleControlPacket(filename, &fileSize, 1 , &controPacketSize);
            
            if(llwrite(controlPacket, controPacketSize) < 0){
                perror("Sending control packet");
                exit(-1);
            }

            int dataSize = leftFileSize > T_SIZE ? T_SIZE : leftFileSize;
            long bytesLeft = fileSize - dataSize;

            while (bytesLeft > 0)
            {   
                dataSize = bytesLeft > T_SIZE ? T_SIZE : bytesLeft;

                int packetSize = dataSize + 4; // Control (1) + Sequence (1) + Length (2) + Data (variable)
                unsigned char dataPacket[packetSize];
                
                assembleDataPacket(packetSize , sequence , dataPacket);
                
                unsigned char *data = getData(file, dataSize);

                memcpy(dataPacket + 4, data, dataSize);

                if(llwrite(dataPacket, dataSize) < 0){
                    perror("Sending data packet");
                    if(nTries == 0){
                        exit(-1);
                    }else{
                        nTries--;
                        continue;
                    }
                }else{
                    nTries = connectionParameters.nRetransmissions;
                    bytesLeft -= dataSize;
                    sequence = (sequence + 1) % 99; //value between 0 and 99  
                }
            }
            
            unsigned char * endControlPacket = assembleControlPacket(filename, &fileSize, 3 , &controPacketSize);
            if(llwrite(endControlPacket, controPacketSize) < 0){
                perror("Sending end control packet");
                exit(-1);
            }

            llclose(0);

            break;
        case LlRx:
            
            unsigned char * packet = (unsigned char *) malloc(T_SIZE * sizeof(unsigned char));
            printf("Waiting for control packet\n");
            int packetSize = llread(packet);

            if(packetSize < 0){
                perror("Receiving control packet");
                exit(-1);
            }

            unsigned char * fileName = malloc(255 * sizeof(unsigned char));
            int filenameSize = extractFileNameAndSize(packet, packetSize , fileName);

            if(filenameSize == -1){
                perror("Error reading name of the file");
                exit(-1);
            }

            FILE * Newfile = fopen(filename, "w+"); //create with w+ for rw access

            if(Newfile == NULL){
                perror("Opening file");
                exit(-1);
            }

            while(1){
                packetSize = llread(packet);
                if(packetSize < 0){
                    perror("Receiving data packet");
                    exit(-1);
                }

                if(packet[1] == 3){
                    break;
                }

                fwrite(packet + 4, sizeof(unsigned char), packetSize - 4, Newfile);
            }

            break;
        default:
            break;
    }

    llclose(0);
    return;
}

unsigned char * assembleControlPacket(const char* filename, long * filesize , int startEnd , int * controlPacketSize ) {
    int filenameLen = strlen(filename);
    
    int bitsNecessary = (int) log2(*filesize + 1); // Calculate the number of bits necessary to store the file size
    int fileLenght = (int) ceil(bitsNecessary / 8.0); // Calculate the number of bytes necessary to store the file size and rounds up
    int packetSize = 1 + 4 + 2 + filenameLen + fileLenght; // C + TLV (file size) + TLV (file name)

    // Allocate space for the control packet
    unsigned char * controlPacket = malloc(packetSize * sizeof(unsigned char));

    int index = 0;

    // 1. Control Field (Start packet)
    controlPacket[index++] = startEnd; // Start control packet

    // 2. File Size TLV
    controlPacket[index++] = 0;       // T: file size
    controlPacket[index++] = 4;      // L: 4 bytes for the integer file size
    controlPacket[index++] = (*filesize >> 24) & 0xFF; // V: file size (MSB first)
    controlPacket[index++] = (*filesize >> 16) & 0xFF;
    controlPacket[index++] = (*filesize >> 8) & 0xFF;
    controlPacket[index++] = *filesize & 0xFF;

    // 3. File Name TLV
    controlPacket[index++] = 1;       // T: file name
    controlPacket[index++] =  strlen(filename) + 1; // T: file name
    memcpy(&controlPacket[index], filename, filenameLen); // V: file name
    index += filenameLen;

    *controlPacketSize = packetSize;
    return controlPacket;
}

int assembleDataPacket(int dataSize , int sequence, unsigned char * dataPacket) {
    int index = 0;
    // 1. Control Field (Data packet)
    dataPacket[index++] = 2; // Control field set to '2' for data

    // 2. Sequence Number (0-99)
    dataPacket[index++] = sequence % 100;

    // 3. Length Fields
    dataPacket[index++] = dataSize >> 8; // MSB
    dataPacket[index++] = dataSize & 0xFF; // LSB (packetSize is 2 bytes)

    return 0; 
}

unsigned char * getData(FILE * file , int dataSize){
    unsigned char * data = (unsigned char *) malloc(dataSize  * sizeof(unsigned char));
    fread(data, sizeof(unsigned char), dataSize, file);
    return data;
}

int extractFileNameAndSize(const unsigned char* packet, int packetSize, unsigned char* fileName) {
    int i = 0;
    int filenameSize = 0;
    int sizeExtracted = 0;
    int nameExtracted = 0;

    while (i < packetSize) {
        if (packet[i] == 0) { // Start of filename segment
            i++;
            filenameSize = packet[i]; // Size of the filename
            i++;

            if (filenameSize + i > packetSize) {
                return -1; // Error: filename size exceeds packet bounds
            }

            // Copy filename
            memcpy(fileName, &packet[i], filenameSize);
            fileName[filenameSize] = '\0'; // Null-terminate
            i += filenameSize;
            nameExtracted = 1;
            sizeExtracted = 1;
        } 
        if (sizeExtracted && nameExtracted) {
            break; // Both file name and size are extracted
        }
    }

    if (!sizeExtracted || !nameExtracted) {
        return -1; // Error: incomplete packet data
    }

    return filenameSize; // Success, return the size of the filename
}

void getFilesize(FILE *file,long *filesize){
    fseek(file, 0, SEEK_END);
    *filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    *filesize -= ftell(file);
    return ;
}
