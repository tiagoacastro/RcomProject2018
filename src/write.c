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

#include "write.h"

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define MAX_ALARM_COUNT 3

#define START_CODE	0x7e
#define END_CODE	0x7e
#define ESCAPE_CODE 0x7d
#define ADDRESS		0x03
#define SET_CODE 	0x03
#define UA_CONTROL  0x07
#define UA_BCC      (ADDRESS ^ UA_CONTROL)

volatile int STOP=FALSE;

int alarm_flag = FALSE;
int alarm_counter = 0;
int message_recieved = FALSE;

int llopen(int fd);

int sendStartMessage(int fd);


void set_alarm(){
	alarm_flag = TRUE;
	alarm_counter++;
}

void remove_alarm(){
	alarm_flag = FALSE;
	alarm_counter = 0;
} 

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;
    
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

	printf("Serial port open\n");

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
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


    //Set the alarm
    //(void) signal(SIGALRM,set_alarm);


	int error = llopen(fd);
	if(error != 0)
		perror("Error with llopen\n");

  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */

   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}

int sendStartMessage(int fd){    
	int res;
	unsigned char message[5];

    message[0] = START_CODE;
	message[1] = ADDRESS;
	message[2] = SET_CODE;
	message[3] = message[1]^message[2];		// calcular a paridade da mensagem
	message[4] = END_CODE;

	printf("Antes de enviar a mensagem\n");

    res = write(fd, message, 5);
    printf("Message sent: %c\n", message[4]);
	if(res <= 0)
    {
        return FALSE;
    }
    else
    {
    	printf("Start Message Written\n");
        return TRUE;
    }
}

/**
 * Recebe uma mensagem e um estado e interpreta em que parte da leitura da trama de supervisao está
 * @param message - Mensagme para ser interpretada
 * @param state - Estado/posiçao na interpretaçao da trama
 */
void stateMachine_Ua(unsigned char *message, int *state){
    switch(*state){

        case 0:                             /* Esta a espera do inicio da trama (0x7E) */
            if(*message == START_CODE){
                *state = 1;
            }
            break;

        case 1:     
            if(*message == ADDRESS){            /* Está à espera do A */
                *state = 2;
            }
            else if(*message == START_CODE){    /* Se tiver um 0x7E no meio da trama */
                *state = 1;
            }
            else{                               /* Houve um erro e Adress está incorreto */
                *state = 0;
            }
            break;

        case 2:
            if(*message == UA_CONTROL){         /* Recebe o valor de Controlo */
                *state = 3;
            }
            else if(*message == START_CODE){   /* Se tiver um 0x7E no meio da trama */
                *state = 1;
            }
            else{                               /* Houve um erro e Controlo não está correto */
                *state = 0;
            }
            break;

        case 3:
            if(*message == UA_BCC){             /* BCC lido com sucesso */
                *state = 4;
            }
            else{                               /* Erro com BCC */
                *state = 0;
            }
            break;

        case 4:
            if(*message == END_CODE){           /* Recebe o ultimo 7E */
                message_recieved = TRUE;
                alarm(0);
                printf("Recebeu o UA\n");
            }
            else{                               /* Erro no 7E */
                *state = 0;
            }     
            break;
    }
} 

int llopen(int fd){
	unsigned char buf[5];
    int error;
    int res;
    int state;

    printf("llopen: entered function\n");

	int i = 0;
    do{

        //Write the starting message and start alarm

		//remove_alarm();
        error = sendStartMessage(fd);
        if(error == FALSE)
            perror("llopen: Error sending starting message\n");
			
		printf("llopen: outside inner loop\n");
		//alarm(TIMEOUT);

        state = 0;          /* Variable that keeps track of the state machine state */
		while(!alarm_flag && !message_recieved){

			printf("llopen : inside inner loop\n");

			res = read(fd,buf+i,1);
			if(res <= 0){
				perror("llopen: read nothing\n");
			}
			else
			{
				printf("llope: Recieved answer\n");
			}
			stateMachine_Ua(&(buf[i]),&state);		
		}
		i++;
	}while(alarm_counter < MAX_ALARM_COUNT && !message_recieved);

    /* Debug */
    printf("alarm_flag: %d\n alarm_counter: %d",alarm_flag,alarm_counter);
    /* Debug */

/*
    if(alarm_flag && alarm_counter == MAX_ALARM_COUNT)
        return FALSE;
    else{
        alarm_flag = FALSE;
        alarm_counter = 0 ;
        return TRUE;
    }
    */

	return 0;
}

int llwrite(int fd, char * buffer, int length) {
    
    return 0;
}

char * concat(const char * s1, const char * s2) {
    
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char * result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

char * packetStuffing(char * buf, int len) {

/*

    char startCodeStuffing[2] = [ESCAPE_CODE, START_CODE ^ 0x20];
    char escapeCodeStuffing[2] = [ESCAPE_CODE, ESCAPE_CODE ^ 0x20];

    char * aux = malloc (1);
    int index, code, auxLength = 1;
    char * ptr;
    int flag = 1;

    while(flag) {

        ptr = strch(buf, START_CODE);
        if (!pos) {
            ptr = strch(buf, ESCAPE_CODE);
            if (!ptr) {
                code = ESCAPE_CODE;
            } else {
                flag = 0;
                continue;
            }
        } else {
            code = START_CODE;
        }

        index = ptr - buf;

        switch(code) {
            case START_CODE:
            break;
            case ESCAPE_CODE:
            default:
            break;
        }


    }

    return aux;

    */
}


