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


int main(int argc,char** argv){ 

    if(argc != 7){
        printf("Usage: %s <serial_port_addr> <operation_mode (w/r)> <baudrate> <nRetries> <timeout> <file_name>\n",argv[0]);
        return -1;
    }

    if(argv[1] != "/dev/ttyS10" && argv[1] != "/dev/ttyS11"){
        printf("Invalid serial port address, please insert '/dev/ttyS10' for transmitter and 'dev/ttyS11' for receiver\n");
        return -1;
    }

    if(strcmp(argv[2],"w") != 0 && strcmp(argv[2],"r") != 0){
        printf("Invalid operation mode, please insert 'w' for sender and 'r' to receiver\n");
        return -1;
    }

    if(atoi(argv[3]) < 0 || argv[3] == NULL){
        printf("Invalid baudrate, please insert a number higher then 0\n");
        return -1;
    }

    if(atoi(argv[4]) < 0 || argv[4] == NULL){
        printf("Invalid number of retries, please insert a number higher then 0\n");
        return -1;
    }

    if(atoi(argv[5]) < 0 || argv[5] == NULL){
        printf("Invalid timeout value, please insert a number higher then 0\n");
        return -1;
    }

    applicationLayer(argv[1],argv[2],atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),argv[6]);

    return 0;
}
