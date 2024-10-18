// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#include "globals.h"
#include "link_layer.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);


unsigned char * assembleControlPacket(const char* filename, long * filesize , int startEnd , int * controlPacketSize);
int assembleDataPacket(int dataSize , int sequence, unsigned char * dataPacket);
unsigned char * getData(FILE * file , int dataSize);
void getFilesize(FILE *file,long *filesize);

#endif // _APPLICATION_LAYER_H_
