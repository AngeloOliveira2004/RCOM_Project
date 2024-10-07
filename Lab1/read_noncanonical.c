// Read from serial port in non-canonical mode
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

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_SIZE 256
#define TRUE 1
#define FALSE 0

// Protocol flags and commands
#define FLAG 0x7E
#define A1 0x03
#define SET 0x03
#define UA 0x07

// Enum for states
enum State {START_STATE, FLAG_RCV_STATE, A_RCV_STATE, C_RCV_STATE, BCC_OK_STATE, STOP_STATE};

// Global stop variable
volatile int STOP = FALSE;

// Error handling function
void handle_error(const char *error_msg) {
    perror(error_msg);
    exit(EXIT_FAILURE);
}

// Set terminal settings for serial communication
void set_termios(int fd, struct termios *old_settings) {
    struct termios new_settings;

    // Save current settings
    if (tcgetattr(fd, old_settings) == -1) {
        handle_error("tcgetattr");
    }

    // Clear and configure new settings
    memset(&new_settings, 0, sizeof(new_settings));
    new_settings.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    new_settings.c_iflag = IGNPAR;
    new_settings.c_oflag = 0;
    new_settings.c_lflag = 0; // Non-canonical mode
    new_settings.c_cc[VTIME] = 0; // No inter-character timer
    new_settings.c_cc[VMIN] = 1;  // Blocking read until 1 char received

    // Flush and apply new settings
    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &new_settings) == -1) {
        handle_error("tcsetattr");
    }
}

// Function to clear buffer
void clear_buffer(unsigned char *buffer, size_t size) {
    memset(buffer, 0, size);
}

// Handle each state transition
void handle_state(enum State *curr_state, unsigned char curr_byte, unsigned char *saved_buf) {
    switch (*curr_state) {
        case START_STATE:
            if (curr_byte == FLAG) {
                *curr_state = FLAG_RCV_STATE;
                saved_buf[0] = curr_byte;
            } else {
                clear_buffer(saved_buf, BUF_SIZE);
            }
            break;

        case FLAG_RCV_STATE:
            if (curr_byte == A1) {
                *curr_state = A_RCV_STATE;
                saved_buf[1] = curr_byte;
            } else if (curr_byte != FLAG) {
                *curr_state = START_STATE;
                clear_buffer(saved_buf, BUF_SIZE);
            }
            break;

        case A_RCV_STATE:
            if (curr_byte == SET) {
                *curr_state = C_RCV_STATE;
                saved_buf[2] = curr_byte;
            } else if (curr_byte == FLAG) {
                *curr_state = FLAG_RCV_STATE;
            } else {
                *curr_state = START_STATE;
                clear_buffer(saved_buf, BUF_SIZE);
            }
            break;

        case C_RCV_STATE:
            if (curr_byte == (saved_buf[1] ^ saved_buf[2])) {
                *curr_state = BCC_OK_STATE;
                saved_buf[3] = curr_byte;
            } else if (curr_byte == FLAG) {
                *curr_state = FLAG_RCV_STATE;
            } else {
                *curr_state = START_STATE;
                clear_buffer(saved_buf, BUF_SIZE);
            }
            break;

        case BCC_OK_STATE:
            if (curr_byte == FLAG) {
                *curr_state = STOP_STATE;
                saved_buf[4] = curr_byte;
            }
            break;

        case STOP_STATE:
            // The state machine has completed successfully
            STOP = TRUE;
            break;
    }
}

// Send UA message after reading SET
void send_UA_message(int fd) {
    unsigned char buf_answer[5] = {FLAG, A1, UA, (A1 ^ UA), FLAG};
    if (write(fd, buf_answer, 5) < 0) {
        handle_error("write");
    }
    printf("Message read and written successfully\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Incorrect usage\nUsage: %s <SerialPort>\nExample: %s /dev/ttyS1\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *serialPortName = argv[1];
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        handle_error(serialPortName);
    }

    struct termios oldtio;
    set_termios(fd, &oldtio);

    unsigned char buf[1];  // Buffer for reading one byte at a time
    unsigned char saved_buf[BUF_SIZE] = {0};
    enum State curr_state = START_STATE;

    // Read loop
    while (!STOP) {
        int bytes = read(fd, buf, 1);
        if (bytes > 0) {
            handle_state(&curr_state, buf[0], saved_buf);
        }
    }

    send_UA_message(fd);

    // Restore old terminal settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        handle_error("tcsetattr");
    }

    close(fd);
    return 0;
}
