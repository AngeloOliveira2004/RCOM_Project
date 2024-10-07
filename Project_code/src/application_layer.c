// Application layer protocol implementation

#include "application_layer.h"

#define FALSE 0
#define TRUE 1
/*

    The application layer will fragment the data and send it one by one to the link layer

    A state machine will be implemented to handle the data transfer and error handling

*/

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

    LinkLayer connectionParameters = {
        .serialPort = *serialPort,
        .role = strcmp(role, "tx") == 0 ? LlTx : LlRx,
        .baudRate = baudRate,
        .nRetransmissions = nTries,
        .timeout = timeout,
    };

    // Open the connection
    int fd = llopen(connectionParameters);

    if (fd < 0)
    {
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
