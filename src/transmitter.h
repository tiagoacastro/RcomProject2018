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

#define RR_0							0x05
#define RR_1							0x85 
#define REJ_0							0x01
#define REJ_1							0x81 

#define APP_CONTROL_SIZE_CONST		0x09
#define APP_CONTROL_DATA	0x01
#define APP_CONTROL_START	0x02
#define APP_CONTROL_END		0x03
#define APP_T_FILESIZE		0x00
#define APP_T_FILENAME		0x01
#define APP_L_FILESIZE		0x04

#define PACKET_SIZE       255

void set_alarm();
void remove_alarm();
unsigned char * openFile(unsigned char * file, int * fileSize);
int sendFile(int fd, unsigned char * fileName, int fileNameSize);
unsigned char * prepareAppControlPacket(unsigned char control, int fileSize, unsigned char * fileName, int fileNameSize, int * appControlPacketSize);
unsigned char * splitData(unsigned char * fileData, int fileSize, int * currPos, int * currPacketSize);
int llopen(int fd);
int llclose(int fd);
int sendControlMessage(int fd, unsigned char control);
int stopAndWaitControl(int fd, unsigned char control_sent, unsigned char control_expecting);
int stopAndWaitData(int fd, unsigned char * buffer, int length);
void stateMachine(unsigned char *message, int *state, unsigned char control);
int llopen(int fd);
int llwrite(int fd, unsigned char * buffer, int length);
unsigned char * concat(const unsigned char * s1, const unsigned char * s2);
unsigned char * packetStuffing(unsigned char * message, int size, int * finalPacketSize);
unsigned char * BCC2Stuffing(unsigned char * bcc2, int * stuffedBCC2Size);
unsigned char generateBCC2(unsigned char *message, int sizeOfMessage);
unsigned char * preparePacket(unsigned char * buf, unsigned char * bcc2, int * finalPacketSize, int * stuffedBCC2Size);

