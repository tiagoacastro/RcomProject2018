/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MAX_ALARM_COUNT 3

#define START_CODE	0x7e
#define END_CODE	0x7e
#define ADRESS		0x03
#define SET_CODE 	0x03

volatile int STOP=FALSE;

int alarm_flag = 0;
int alarm_counter = 0;
int message_recieved = 0;

int llopen(int fd);


void set_alarm(){
	alarm_flag = 1;
	alarm_counter++;
}

void remove_alarm(){
	alarm_flag = 0;
	alarm_counter = 0;
} 

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


    //Set the alarm
    (void) signal(SIGALRM,set_alarm);

	int error = llopen(fd);
	if(error != 0)
		perror("Error with llopen\n");

  /* 
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar 
    o indicado no guião 
  */

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }




    close(fd);
    return 0;
}

int llopen(int fd){
	int res;
	char message[5];
	char buf[5];

	message[0] = START_CODE;
	message[1] = ADRESS;
	message[2] = SET_CODE;
	message[3] = message[1]^message[2];		// calcular a paridade da mensagem
	message[4] = END_CODE;


	int i = 0;
	while(alarm_counter < MAX_ALARM_COUNT && !message_recieved){

		remove_alarm();

		res = write(fd, message, 5);
		if(res <= 0)
			perror("Error writing setup message");	

		alarm(3);

		while(!alarm_flag && !message_recieved){

			res = read(fd,buf+i,1);
			if(res > 0)
			{
				if(buf[i] == END_CODE){
					message_recieved = 1;
					remove_alarm();
				}	
				i++;
			}
		
		}

	}
	return 0;
}


