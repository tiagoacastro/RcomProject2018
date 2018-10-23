/*Non-Canonical Input Processing*/

#include "write.h"

volatile int STOP=FALSE;

int alarm_flag = FALSE;
int alarm_counter = 0;
int message_received = FALSE;

void alarm_handler() { 
	alarm_flag = TRUE;
	alarm_counter++;
	//printf("alarm counter: %d\n", alarm_counter);
}

void reset_alarm_flag(){
  alarm_flag = FALSE;
}

void reset_alarm_counter() {
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

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



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
    signal(SIGALRM,alarm_handler);


	int error = llopen(fd);
	if(error != 0){
		perror("Error with llopen\n");
        exit(1);
    }

	llclose(fd);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}

int sendControlMessage(int fd, unsigned char control){
	int res;
	unsigned char message[5];

  message[0] = FLAG;
	message[1] = ADDRESS;
	message[2] = control;
	message[3] = message[1]^message[2];		// calcular a paridade da mensagem
	message[4] = FLAG;

	//printf("Antes de enviar a mensagem\n");

    res = write(fd, message, 5);
    printf("Message sent: 0x%02x\n", message[2]);
	if(res <= 0)
    {
        return FALSE;
    }
    else
    {
    	printf("Control message sent\n");
        return TRUE;
    }
}

/**
 * Recebe uma mensagem e um estado e interpreta em que parte da leitura da trama de supervisao está
 * @param message - Mensagme para ser interpretada
 * @param state - Estado/posiçao na interpretaçao da trama
 */
void stateMachine(unsigned char *message, int *state, unsigned char control){
    switch(*state){

        case 0:                             /* Esta a espera do inicio da trama (0x7E) */
            if(*message == FLAG){
                *state = 1;
            }
            break;

        case 1:
            if(*message == ADDRESS){            /* Está à espera do A */
                *state = 2;
            }
            else if(*message == FLAG){    /* Se tiver um 0x7E no meio da trama */
                *state = 1;
            }
            else{                               /* Houve um erro e Adress está incorreto */
                *state = 0;
            }
            break;

        case 2:
            if(*message == control){         /* Recebe o valor de Controlo */
                *state = 3;
            }
            else if(*message == FLAG){   /* Se tiver um 0x7E no meio da trama */
                *state = 1;
            }
            else{                               /* Houve um erro e Controlo não está correto */
                *state = 0;
            }
            break;

        case 3:
            if(*message == (control ^ ADDRESS)){             /* BCC lido com sucesso */
                *state = 4;
            }
            else{                               /* Erro com BCC */
                *state = 0;
            }
            break;

        case 4:
            if(*message == FLAG){           /* Recebe o ultimo 7E */
                message_received = TRUE;
                alarm(0);
                printf("Recebeu 0x%02x\n", control);
            }
            else{                               /* Erro no 7E */
                *state = 0;
            }
            break;
    }
}

int stopAndWaitControl(int fd, unsigned char control_sent, unsigned char control_expecting) {

	printf("entered stop and wait\n");

	int res = 0;
	unsigned char buf[1];
	int state = 0;

	reset_alarm_flag();
	reset_alarm_counter();
	message_received = FALSE;
	
	while(alarm_counter < MAX_ALARM_COUNT && !message_received){
		
		sendControlMessage(fd, control_sent);
		printf("sent control message, waiting response\n");
		alarm(TIMEOUT);
		state = 0;
		while(!alarm_flag && !message_received) {
				res = read(fd, buf, 1);
				if(res <= 0){
					//perror("stopAndWait: read nothing\n");
				}
				else
				{
					printf("stopAndWait: Received answer: 0x%02x\n", *buf);
				} 	
				stateMachine(buf, &state, control_expecting);
		}
		reset_alarm_flag();

        printf("Alarm counter %d\n", alarm_counter);

	};

    if(alarm_counter < MAX_ALARM_COUNT){
        return 0;
    }
    else
        return 1;
  
	
}


int llopen(int fd){
	return stopAndWaitControl(fd, CONTROL_SET,CONTROL_UA);
}

int llclose(int fd) {
	stopAndWaitControl(fd, CONTROL_DISC,CONTROL_DISC);
	sendControlMessage(fd, CONTROL_UA);

    return 0;
}

/**
* Calculate the parity of the message of the argument
* @param message - pointer to the message which parity wants to be calculated
* @param size - size of the message sent
* @returns the parity of the message in form of a unsigned char
*/
unsigned char generateBCC2(unsigned char *message, unsigned int sizeOfMessage){

    unsigned char result = message[0];

    int i;
    for(i = 1; i < sizeOfMessage; i++){
        result = result ^ message[i]; 
    }

    return result;
}

/**
* 
*/
int llwrite(int fd, char * buffer, int length) {

    //bcc2
    unsigned char BCC2 = generateBCC2(buffer,length);
    int sizeBCC2 = 1;
    unsigned char *stuffedBCC2 = (unsigned char *)malloc(sizeof(unsigned char));

    //create packet header 

    //stuffing

    //concatenate packet header and stuffed message

    //sending packages of the file at once

    return 0;
}

unsigned char * concat(const unsigned char * s1, const unsigned char * s2) {

    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char * result = malloc(len1 + len2 + 1);
    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);
    return result;
}

/**
*   Stuffs the message in buf to prevent it from having the char FLAG which would cause a problem while reading
*   @param buf - message to be stuffed
*   @param len - length of the message to be stuffed
*   @return the message but stuffed (with no FLAGS in it)
*/
unsigned char *packetStuffing(unsigned char * message, int size) {
    //Counter for the original message
    int i;
    //Counter for the new message
    int j;
    int sizeFinalMessage = 1;
    unsigned char *finalMessage = (unsigned char *)malloc(sizeof(unsigned char));

    //Loop through all the old message and generate the new one
    for(i = 0; i < size ; i++){
        if(message[i] == FLAG){
            finalMessage = (unsigned char *)realloc(finalMessage,++sizeFinalMessage);
            finalMessage[j] = ESCAPE_CODE;
            finalMessage[j+1] = STUFF_FLAG_CODE;   
            j += 2;
        }
        else if(message[i] == ESCAPE_CODE){
            finalMessage = (unsigned char *)realloc(finalMessage,++sizeFinalMessage);
            finalMessage[j] = ESCAPE_CODE;
            finalMessage[j+1] = STUFF_ESCAPE_CODE;

            j+=2;
        }
        else{
            finalMessage[j] = message[i];
            j++;
        }
    }

	return finalMessage;
}

