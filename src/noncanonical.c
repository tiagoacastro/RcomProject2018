/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int llopen(int fd){
    char buf, message[5];
    int state = 0;
    int stop = 0;
    int i = 0;

    while (stop = 0) {       
      	res = read(fd,buf,1);
	if(res > 0){
		switch(state){
			case 0:
				if(buf == 0x7E){
					state++;
					message[i] = 0x7E; 
					i++;
				}
				break;				
			case 1:
				if(buf != 0x7E){
					if(i == 4) {
						i = 0;
						state = 0;
					}
					message[i] = buf; 
					i++;
				} else {
					if(i == 4){
						message[i] = 0x7E;
						stop = 1;
					} else {
						i = 0;
						state = 0;
					}
				}
				break;		
		}
	}
    }

    char bcc1 = message[1] ^ message[2];

    if(bcc1 != message[3] || message[2] != 0x03)
    	return -1;
	
    //write AU

    return 0;
}

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255], echo[255];

    strcpy(echo,"");

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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



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

    llopen(fd);  	//llopen called here

/*
    while (STOP==FALSE) {       
      res = read(fd,buf,255);   
      buf[res]=0;               
      printf(":%s:%d\n", buf, res);
      if (buf[res-1]=='\0') STOP=TRUE;
      strcat(echo, buf);
    }

    res = write(fd, echo, strlen(echo) + 1);
    printf("%d bytes echoed\n", res);

    sleep(2);
*/
  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião 
  */

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
