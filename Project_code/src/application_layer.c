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
            FILE *file = fopen(filename, "rb");
            if (file == NULL) {
                perror("Opening file");
                exit(-1);
            }

            long fileSize;
            getFilesize(file, &fileSize);

            long bytesLeft = fileSize;
            int controPacketSize = 0;
            unsigned char *controlPacket = assembleControlPacket(filename, &fileSize, 1 , &controPacketSize);

            if (llwrite(controlPacket, controPacketSize) < 0) {
                perror("Sending control packet");
                exit(-1);
            }

            while (bytesLeft > 0) {   
                printf("Bytes left: %ld\n", bytesLeft);

                int dataSize = (bytesLeft > T_SIZE) ? T_SIZE : bytesLeft;
                int packetSize = dataSize + 4;

                unsigned char dataPacket[packetSize];
                assembleDataPacket(dataSize, sequence, dataPacket);

                if (fread(dataPacket + 4, sizeof(unsigned char), dataSize, file) != dataSize) {
                    if (feof(file)) {
                        fprintf(stderr, "End of file reached prematurely.\n");
                    } else {
                        perror("fread error");
                    }
                    exit(-1);
                }

                if (llwrite(dataPacket, packetSize) < 0) {
                    perror("Sending data packet");
                    exit(-1);
                }

                bytesLeft -= dataSize;
                sequence = (sequence + 1) % 99;
                /* 
                // create a progress bar based on bytesLeft and fileSize
                system("clear");
                printf("\n\n\n\n\n\n");
                printf("\rProgress: [");
                for (int i = 0; i < 50; i++) {
                    if (i < (50 * (1 - (bytesLeft / (float) fileSize)))) {
                        printf("=");
                    } else {
                        printf(" ");
                    }
                }
                printf("] %.2f%%", 100 * (1 - (bytesLeft / (float) fileSize)));
                */
                fflush(stdout);
            }

            
            unsigned char * endControlPacket = assembleControlPacket(filename, &fileSize, 3 , &controPacketSize);
            if(llwrite(endControlPacket, controPacketSize) < 0){
                perror("Sending end control packet");
                exit(-1);
            }

            printf("End of transmission\n");

            break;
        case LlRx:
            
            unsigned char * packet = (unsigned char *) malloc(2*T_SIZE * sizeof(unsigned char));

            int packetSize = llread(packet);

            long fileSizeReceiver = 0;

            long bytesLeftReceiver = 0;

            if(packet[0] == 1){
                fileSizeReceiver = (packet[3] << 24) | (packet[4] << 16) | (packet[5] << 8) | packet[6];
                bytesLeftReceiver = fileSizeReceiver;
                printf("File size: %ld\n", fileSizeReceiver);
            }

            if(packetSize < 0){
                perror("Receiving control packet");
                exit(-1);
            }

            FILE * Newfile = fopen(filename, "wb+"); //create with w+ for rw access

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

                if(packet[0] == 3) break;
                else if(packet[0] != 2) continue;   

                if (packetSize <= 0) {
                    fprintf(stderr, "Invalid packet size\n");
                    exit(-1);
                }

                if (fwrite(packet + 4, sizeof(unsigned char), packetSize - 4, Newfile) != packetSize - 4) { // I don't want to write the first 4 bytes of the packet
                    perror("Error writing to file");
                    exit(-1);
                }

                bytesLeftReceiver -= (packetSize - 4);
                /*
                system("clear");
                printf("\n\n\n\n\n\n");
                printf("\rProgress: [");
                for (int i = 0; i < 50; i++) {
                    if (i < (50 * (1 - (bytesLeftReceiver / (float) fileSizeReceiver)))) {
                        printf("=");
                    } else {
                        printf(" ");
                    }
                }
                printf("] %.2f%%", 100 * (1 - (bytesLeftReceiver / (float) fileSizeReceiver)));
                fflush(stdout);
                */

                //create a progress bar based on fileSizeReceiver and T_SIZE

                fflush(Newfile);

                memset(packet, 0, 2 * T_SIZE);
            }

            printf("End of file\n");

            fclose(Newfile);

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

void getFilesize(FILE *file,long *filesize){
    fseek(file, 0, SEEK_END);
    *filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    *filesize -= ftell(file);
    return ;
}
