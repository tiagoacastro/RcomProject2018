#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <signal.h>

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */
#define BAUDRATE        B38400
#define _POSIX_SOURCE   1 /* POSIX compliant source */
#define FALSE           0
#define TRUE            1

#define MAX_ALARM_COUNT 3
#define TIMEOUT         5

#define FLAG            0x7E
#define C_SET           0x03
#define C_UA            0x07
#define A               0x03
#define C_DISC					0x0B
#define C_0							0x00
#define C_1 						0x40
#define ESC							0x7D
#define ESC_FLAG				0x5E
#define ESC_ESC					0x5D

int llOpen(int fd);
int llClose(int fd);
void readControlMessage(int fd, unsigned char control);
void writeControlMessage(int fd, unsigned char control);
int llread(int fd, unsigned char *buffer);
