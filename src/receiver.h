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

#define TIMEOUT					3

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
#define RR_C_0          0x05
#define RR_C_1          0x85
#define REJ_C_0         0x01
#define REJ_C_1         0x81
#define DATA   	        0x01
#define START   	      0x02
#define END   	        0x03
#define SIZE   	        0x00
#define NAME   	        0x01

struct FileInfo{
  unsigned int size;
  unsigned char* name;
  unsigned char* content;
};

int llOpen(int fd);
int llClose(int fd);
void readControlMessage(int fd, unsigned char control);
void writeControlMessage(int fd, unsigned char control);
unsigned int llread(int fd, unsigned char* buffer);
int destuffing(unsigned char* buffer, unsigned int packetSize);
int checkBCC2(unsigned char* buffer, unsigned int packetSize);
unsigned int readPacket(int fd, unsigned char* buffer);
int getFileInfo(unsigned char* start);
void readContent(int fd, unsigned char* start, unsigned int startSize);
unsigned int isEndPacket(unsigned char* start, unsigned int startSize, unsigned char* end, unsigned int endSize);
unsigned int removeHeader(unsigned char* packet, unsigned int size);
void createFile();
