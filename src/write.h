#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TIMEOUT         3

#define BAUDRATE        B38400
#define MODEMDEVICE     "/dev/ttyS1"
#define _POSIX_SOURCE   1 /* POSIX compliant source */
#define FALSE           0
#define TRUE            1

#define MAX_ALARM_COUNT 3

#define FLAG				    0x7e
#define ADDRESS		      0x03
#define CONTROL_SET 	  0x03
#define CONTROL_UA      0x07
#define CONTROL_DISC   	0x0b
#define BCC_UA          (ADDRESS ^ CONTROL_UA)
#define PACKET_HEADER_SIZE	6		//Number of bytes used around an information packet 

#define ESCAPE_CODE     	0x7d
#define STUFF_FLAG_CODE		0x5e
#define STUFF_ESCAPE_CODE	0x5d

#define A_WRITER					0x03

void set_alarm();
void remove_alarm();
int llopen(int fd);
int llclose(int fd);
int sendControlMessage(int fd, unsigned char control);
void stateMachine(unsigned char *message, int *state, unsigned char control);
int llopen(int fd);
int llwrite(int fd, unsigned char * buffer, int length);
unsigned char * concat(const unsigned char * s1, const unsigned char * s2);
unsigned char * packetStuffing(unsigned char * buf, int len);
unsigned char generateBCC2(unsigned char *message, int sizeOfMessage);
void preparePacket(unsigned char * buf, unsigned char bcc2);

