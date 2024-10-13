// Application layer protocol implementation

#include "application_layer.h"

#define FALSE 0
#define TRUE 1
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

    // Open the file
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Opening file");
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



            fseek(file, 0, SEEK_END);
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

int assembleControlPacket(int fd, char* filename, int* filesize , int startEnd) {
    int filenameLen = strlen(filename);
    int packetSize = 1 + 2 + 4 + 2 + filenameLen; // C + TLV (file size) + TLV (file name)

    // Allocate space for the control packet
    unsigned char controlPacket[packetSize];

    int index = 0;

    // 1. Control Field (Start packet)
    controlPacket[index++] = startEnd; // Start control packet

    // 2. File Size TLV
    controlPacket[index++] = 0;       // T: file size
    controlPacket[index++] = 4;       // L: 4 bytes for the integer file size
    controlPacket[index++] = (*filesize >> 24) & 0xFF; // V: file size (MSB first)
    controlPacket[index++] = (*filesize >> 16) & 0xFF;
    controlPacket[index++] = (*filesize >> 8) & 0xFF;
    controlPacket[index++] = *filesize & 0xFF;

    // 3. File Name TLV
    controlPacket[index++] = 1;                 // T: file name
    controlPacket[index++] = filenameLen & 0xFF; // L: length of the file name string
    memcpy(&controlPacket[index], filename, filenameLen); // V: file name
    index += filenameLen;

    // Send the packet
    int bytesSent = 0;
    int bytesToSend = index;
/* 
    while (bytesSent < bytesToSend) {
        int bytesSentNow = write(fd, controlPacket + bytesSent, bytesToSend - bytesSent);
        if (bytesSentNow < 0) {
            perror("Sending data failed");
            exit(-1);
        }
        bytesSent += bytesSentNow;
    }
*/
    return bytesSent; // Return the total bytes sent
}

int assembleDataPacket(int fd, FILE *file , unsigned char* dataPacket) {
    // Define the maximum payload size (subtracting the control, sequence, and length bytes)
    int maxPayloadSize = T_SIZE - 4;
    int sequenceNumber = 99;

    // Buffer to hold the data read from the file
    unsigned char dataBuffer[maxPayloadSize];
    int bytesRead = fread(dataBuffer, 1, maxPayloadSize, file);
    
    if (bytesRead <= 0) {
        return 0; // End of file or read error
    }

    // Calculate the length in two bytes (L2, L1)
    unsigned char L2 = (bytesRead >> 8) & 0xFF;
    unsigned char L1 = bytesRead & 0xFF;

    int index = 0;

    // 1. Control Field (Data packet)
    dataPacket[index++] = 2; // Control field set to '2' for data

    // 2. Sequence Number (0-99)
    dataPacket[index++] = sequenceNumber % 100;

    // 3. Length Fields
    dataPacket[index++] = L2;
    dataPacket[index++] = L1;

    // 4. Data Field (Payload)
    memcpy(&dataPacket[index], dataBuffer, bytesRead);
    index += bytesRead;

    return 0; 
}


// int main(int argc,char** argv){ 

//     if(argv[1] != "/dev/ttyS10" || argv[1] != "/dev/ttyS11"){
//         printf("Invalid serial port address, please insert '/dev/ttyS10' for transmitter and 'dev/ttyS11' for receiver\n");
//         return -1;
//     }

//     if(strcmp(argv[2],"w") != 0 || strcmp(argv[2],"r") != 0){
//         printf("Invalid operation mode, please insert 'w' for sender and 'r' to receiver\n");
//         return -1;
//     }

//     if(atoi(argv[3]) < 0 || argv[3] == NULL){
//         printf("Invalid baudrate, please insert a number higher then 0\n");
//         return -1;
//     }

//     if(atoi(argv[4]) < 0 || argv[4] == NULL){
//         printf("Invalid number of retries, please insert a number higher then 0\n");
//         return -1;
//     }

//     if(atoi(argv[5]) < 0 || argv[5] == NULL){
//         printf("Invalid timeout value, please insert a number higher then 0\n");
//         return -1;
//     }

//     applicationLayer(argv[1],argv[2],atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),argv[6]);

//     return 0;
// }


int getFilesize(FILE *file,int *filesize){
    fseek(file, 0, SEEK_END);
    *filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return 0;
}
