/*Non-Canonical Input Processing*/
#include "transmitter.h"

int packetSign = 0;
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
    leitura do(s) proximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");


  //Set the alarm

  signal(SIGALRM,alarm_handler);

	//
	//LLOPEN
	//

	int error = llopen(fd);
	if(error != 0){
		perror("Error with llopen\n");
        exit(1);
  }

	//
	//SENDFILE (LLWRITE)
	//

	sendFile(fd);

	//
	//LLCLOSE
	//

	llclose(fd);

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}

//
// Application related functions
//

int sendFile(int fd) {

	unsigned char testFile[12];
	testFile[0] = 'p';
	testFile[1] = 'i';
	testFile[2] = 'n';
	testFile[3] = 'g';
	testFile[4] = 'u';
	testFile[5] = 'i';
	testFile[6] = 'm';
	testFile[7] = '.';
	testFile[8] = 'g';
	testFile[9] = 'i';
	testFile[10] = 'f';
	testFile[11] = '\0';

	unsigned char testPacket[6];
	testPacket[0] = 't';
	testPacket[1] = 'e';
	testPacket[2] = 0x7e;
	testPacket[3] = 's';
	testPacket[4] = 't';
	testPacket[5] = '\0';

	//loop through these steps for as many times as the number of packets the file is divide into

	int stopAndWaitDataReturn;

	stopAndWaitDataReturn = stopAndWaitData(fd, testPacket, 6);
	switch(stopAndWaitDataReturn) {
	case 0:
		break;
	case 1:
		//timed out, call llclose
		break;
	case 2:
		//packet rejected, send it again 
		stopAndWaitData(fd, testPacket, 6);
	}

	sleep(2);

	return 0;
}

unsigned char * openFile(unsigned char * file, int * fileSize) {

	FILE * f;
	struct stat metadata;
	unsigned char * fileData;

	if ((f = fopen((char *) file, "rb")) == NULL) {
		perror("Error opening file\n");
	}	

	stat((char *) file, &metadata);
	(*fileSize) = metadata.st_size;

	fileData = (unsigned char *) malloc(*fileSize);
	fread(fileData, sizeof(unsigned char), *fileSize, f);
	return fileData;
}

unsigned char * prepareAppControlPacket(unsigned char control, int fileSize, unsigned char * fileName, int fileNameSize, int * appControlPacketSize) {

	(* appControlPacketSize) = 9 * sizeof(unsigned char) + fileNameSize;
	unsigned char * controlPacket = (unsigned char *) malloc (*appControlPacketSize);

 	controlPacket[0] = control;
	controlPacket[1] = APP_T_FILESIZE;
	controlPacket[2] = APP_L_FILESIZE;
	controlPacket[3] = (fileSize >> 24) & 0xFF;
	controlPacket[4] = (fileSize >> 16) & 0xFF;
	controlPacket[5] = (fileSize >> 8) & 0xFF;
	controlPacket[6] = fileSize & 0xFF;
	controlPacket[7] = APP_T_FILENAME;
	controlPacket[8] = fileNameSize;

	for (int i = 0; i < fileNameSize; i++) {
		controlPacket[9+i] = fileName[i];
	}

	return controlPacket;

}

unsigned char * prepareDataPacketHeader(unsigned char * data, int fileSize, int * packetSize, int * numPackets) {
	
	unsigned char * finalDataPacket = (unsigned char *) malloc(fileSize+4);

	finalDataPacket[0] = APP_CONTROL_DATA;
	finalDataPacket[1] = (*numPackets) % 255;
	finalDataPacket[2] = fileSize / 256;
	finalDataPacket[3] = fileSize % 256;

	memcpy(finalDataPacket+4, data, *packetSize);
	(*packetSize) += 4;

	(*numPackets)++;
	return finalDataPacket;

}

// podem faltar argumentos!
unsigned char * splitAndSendData(unsigned char * data){
	return 0;
}


//
// Data link layer related functions
//

int sendControlMessage(int fd, unsigned char control) {

	int res;
	unsigned char message[5];

  message[0] = FLAG;
	message[1] = ADDRESS;
	message[2] = control;
	message[3] = message[1]^message[2];		// calcular a paridade da mensagem
	message[4] = FLAG;

	//printf("Antes de enviar a mensagem\n");

  res = write(fd, message, 5);
	if(res <= 0)
  {
      return FALSE;
  }
  else
  {
  	printf("Control message sent: 0x%02x\n", message[2]);
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
            printf("Received 0x%02x\n", control);
        }
        else{                               /* Erro no 7E */
            *state = 0;
        }
        break;
  }
}

int stopAndWaitControl(int fd, unsigned char control_sent, unsigned char control_expecting) {

	//printf("entered stop and wait\n");

	int res = 0;
	unsigned char buf[1];
	int state = 0;

	reset_alarm_flag();
	reset_alarm_counter();
	message_received = FALSE;

	while(alarm_counter < MAX_ALARM_COUNT && !message_received){

		sendControlMessage(fd, control_sent);
		//printf("sent control message, waiting response\n");
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

    //printf("Alarm counter %d\n", alarm_counter);

	};

  if(alarm_counter < MAX_ALARM_COUNT){
      return 0;
  }
  else
      return 1;
}

void stateMachineData(unsigned char *message, int *state, unsigned char * controlReceived){
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
        if(*message == RR_0 || *message == RR_1 || *message == REJ_0 || *message == REJ_1){         /* Recebe o valor de Controlo */
						*controlReceived = (*message);
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
        if(*message == ((*controlReceived) ^ ADDRESS)){             /* BCC lido com sucesso */
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
        }
        else{                               /* Erro no 7E */
            *state = 0;
        }
        break;
  }
}

int stopAndWaitData(int fd, unsigned char * buffer, int length) {

	int res = 0;
	unsigned char buf[1];
	int state = 0;
	unsigned char controlReceived[1];

	reset_alarm_flag();
	reset_alarm_counter();
	message_received = FALSE;

	//send the message

	llwrite(fd, buffer, length);

	while(alarm_counter < MAX_ALARM_COUNT && !message_received){

		alarm(TIMEOUT);
		while(!alarm_flag && !message_received) {
			res = read(fd, buf, 1); 
			if(res <= 0){
				//perror("stopAndWait: read nothing\n");
			}
			else
			{
				printf("stopAndWait: Received answer: 0x%02x\n", *buf);
				stateMachineData(buf, &state, controlReceived);
			}
		}

		//re-send the data packet
		if (alarm_flag) {
			llwrite(fd, buffer, length);
		}

		reset_alarm_flag();

	};

  if(alarm_counter < MAX_ALARM_COUNT) {

		switch(controlReceived[0]) {
		case RR_0:
			packetSign = 1;
			return 0;
		case RR_1:
			packetSign = 0;
			return 0;
		case REJ_0:
			return 2;
		case REJ_1:
			return 2;
		}

  } else
  	return 1;
}


int llopen(int fd){
	return stopAndWaitControl(fd, CONTROL_SET,CONTROL_UA);
}

int llclose(int fd) {
	stopAndWaitControl(fd, CONTROL_DISC,CONTROL_DISC);
	//if it doesnt respond to the disc signal, it shouldnt try to send the ua
	sendControlMessage(fd, CONTROL_UA);
  return 0; //shouldn't just be returning 0
}

/**
* Calculate the parity of the message of the argument
* @param message - pointer to the message which parity wants to be calculated
* @param size - size of the message sent
* @returns the parity of the message in form of a unsigned char
*/

unsigned char generateBCC2(unsigned char *message, int sizeOfMessage){

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
int llwrite(int fd, unsigned char * buffer, int length) {

	int * finalPacketSize = (int *) malloc(sizeof(int));
	int * stuffedBCC2Size = (int *) malloc(sizeof(int));
	unsigned char * finalPacket = (unsigned char *) malloc(sizeof(unsigned char));

	//
  //bcc2
	//

  unsigned char BCC2 = generateBCC2(buffer,length);
  unsigned char * stuffedBCC2 = (unsigned char *) malloc(sizeof(unsigned char));	

	//
  //stuffing
	//

	stuffedBCC2 = BCC2Stuffing(&BCC2, stuffedBCC2Size);
	finalPacket = packetStuffing(buffer, length, finalPacketSize);

	//
  //concatenate packet header, stuffed message and packet trailer
	//

	finalPacket = preparePacket(finalPacket, stuffedBCC2, finalPacketSize, stuffedBCC2Size); 
	printf("Packet prepared, current packet:\n");

	for (int i = 0; i < (*finalPacketSize); i++) {
		printf("0x%02x\n", finalPacket[i]);
	}

	//
  //sending packages of the file at once
	//

	printf("Preparing to send packet\n");
	int num = write(fd, finalPacket, *finalPacketSize); 
	printf("Number of chars sent: %i\n", num);

	//
	//waiting for response from receiver
	//

	

	free(finalPacketSize);
	free(stuffedBCC2);
	free(finalPacket);
  return 0;
}

unsigned char * BCC2Stuffing(unsigned char * bcc2, int * stuffedBCC2Size) {
	
	unsigned char * stuffedBCC2 = (unsigned char *) malloc(sizeof(unsigned char));

	if((*bcc2) == FLAG){
    stuffedBCC2 = (unsigned char *)realloc(stuffedBCC2,2);
    stuffedBCC2[0] = ESCAPE_CODE;
    stuffedBCC2[1] = STUFF_FLAG_CODE;
		(*stuffedBCC2Size) = 2;
	}
	else if((*bcc2) == ESCAPE_CODE) {
    stuffedBCC2 = (unsigned char *)realloc(stuffedBCC2,2);
   	stuffedBCC2[0] = ESCAPE_CODE;
    stuffedBCC2[1] = STUFF_ESCAPE_CODE;
		(*stuffedBCC2Size) = 2;
	}
	else {
	  stuffedBCC2[0] = (*bcc2);
		(*stuffedBCC2Size) = 1;
	}

	return stuffedBCC2;
}

/**
*   Stuffs the message in buf to prevent it from having the char FLAG which would cause a problem while reading
*   @param buf - message to be stuffed
*   @param len - length of the message to be stuffed
*   @return the message but stuffed (with no FLAGS in it)
*/
unsigned char * packetStuffing(unsigned char * message, int size, int * finalPacketSize) {

	printf("Started packet stuffing\n");

  //Counter for the original message
  int i;
  //Counter for the new message
  int j = 0;
  int sizeFinalMessage = 0;
  unsigned char *finalMessage = (unsigned char *)malloc(sizeof(unsigned char));

  //Loop through all the old message and generate the new one
  for(i = 0; i < size ; i++) {

		printf("Analyzing packet member: 0x%02x\n", message[i]);

		if(message[i] == FLAG){
			sizeFinalMessage += 2;
	    finalMessage = (unsigned char *)realloc(finalMessage,sizeFinalMessage);
	    finalMessage[j] = ESCAPE_CODE;
	    finalMessage[j+1] = STUFF_FLAG_CODE;
	    j += 2;
		}
		else if(message[i] == ESCAPE_CODE) {
			sizeFinalMessage += 2;
	    finalMessage = (unsigned char *)realloc(finalMessage,sizeFinalMessage);
	    finalMessage[j] = ESCAPE_CODE;
	    finalMessage[j+1] = STUFF_ESCAPE_CODE;
	    j += 2;
		}
		else {
			sizeFinalMessage++;
			finalMessage = (unsigned char *)realloc(finalMessage,sizeFinalMessage);
		  finalMessage[j] = message[i];
		  j++;
		}
  }

	for (int i = 0; i < sizeFinalMessage; i++) {
		printf("0x%02x\n", finalMessage[i]);
	}
	
	(*finalPacketSize) = sizeFinalMessage;
	return finalMessage;
}

unsigned char * preparePacket(unsigned char * buf, unsigned char * bcc2, int * finalPacketSize, int * stuffedBCC2Size) {

	unsigned char header[4];
	header[0] = FLAG;
	header[1] = A_WRITER;
	header[2] = packetSign;
	header[3] = header[1] ^ header[2];

	unsigned char trailer[1];
	trailer[0] = FLAG;

	unsigned char * auxBuf = (unsigned char *) malloc((*finalPacketSize) * sizeof(unsigned char));

	memcpy(auxBuf, header, 4);
	memcpy(auxBuf+4, buf, (*finalPacketSize));
	memcpy(auxBuf+4+(*finalPacketSize), bcc2, (*stuffedBCC2Size));
	memcpy(auxBuf+4+(*finalPacketSize)+(*stuffedBCC2Size), trailer, 1);

	(*finalPacketSize) += 5 + (*stuffedBCC2Size); // flag, address, control, bcc1, bcc2, flag

	printf("Final packet size in preparePacket: %i\n", *finalPacketSize);

	return auxBuf;

}
