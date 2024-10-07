// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
//#include "alarm.c"

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define FLAG 0x7E

#define A1 0x03

#define A2 0x01

#define SET 0x03

#define UA 0x07

#define BUF_SIZE 256
#define MSG_SIZE 5

volatile int STOP = FALSE;

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    // Create string to send
    unsigned char set_message[MSG_SIZE] = {FLAG, A1, SET, A1 ^ SET, FLAG};
    unsigned char ua_message[MSG_SIZE] = {0};

    // Initial write of the SET message
    int bytes_written = write(fd, set_message, MSG_SIZE);
    if (bytes_written != MSG_SIZE) {
        perror("write");
        exit(-1);
    }
    printf("%d bytes written\n", bytes_written);

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    // buf[5] = '\n';

    // Wait until all bytes have been written to the serial port
    sleep(1);

    while (alarmCount < 4) {
        if (alarmEnabled == FALSE) {
            alarm(3);
            signal(SIGALRM, alarmHandler);

            // Resend the SET message after the alarm triggers
            bytes_written = write(fd, set_message, MSG_SIZE);
            if (bytes_written != MSG_SIZE) {
                perror("write");
                exit(-1);
            }
            printf("Resent %d bytes\n", bytes_written);
            alarmEnabled = TRUE;
        }

        // Try reading the UA message
        int bytes_read = read(fd, ua_message, MSG_SIZE);
        if (bytes_read > 0) {
            printf("Read %d bytes\n", bytes_read);
            if (ua_message[0] == FLAG && ua_message[1] == A1 &&
                ua_message[2] == UA && ua_message[3] == (A1 ^ UA) && 
                ua_message[4] == FLAG) {
                printf("UA received successfully\n");

                // Turn off the alarm and exit the loop
                alarmEnabled = FALSE;
                STOP = TRUE;
                break;
            }
        } else {
            perror("read");
        }
    }

    if (!STOP) {
        printf("Message was not received after 4 retries\n");
    }

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}