#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */
#define BAUDRATE      B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE         0
#define TRUE          1

#define FLAG          0x7E
#define C_SET         0x03
#define C_UA          0x07
#define A             0x03

//data link layer
int llOpen();
int llClose();
int readControlMessage(int fd, unsigned char control);
void sendControlMessage(int fd, unsigned char C);
