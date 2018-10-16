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

#define START_CODE	    0x7e
#define END_CODE	      0x7e
#define ESCAPE_CODE     0x7d
#define ADDRESS		      0x03
#define SET_CODE 	      0x03
#define UA_CONTROL      0x07
#define UA_BCC          (ADDRESS ^ UA_CONTROL)

void set_alarm();
void remove_alarm();
int llopen(int fd);
int sendStartMessage(int fd);
void stateMachine_Ua(unsigned char *message, int *state);
int llopen(int fd);
int llwrite(int fd, char * buffer, int length);
char * concat(const char * s1, const char * s2);
char * packetStuffing(char * buf, int len);
