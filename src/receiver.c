/*Non-Canonical Input Processing*/
#include "receiver.h"

struct termios oldtio, newtio;
int packet;
int expected = 0;
struct FileInfo info;
unsigned char* changed;

int alarm_flag = FALSE;
int alarm_counter = 0;
int message_received = FALSE;

//
// ALARM HANDLER 
//

void alarm_handler() {
	alarm_flag = TRUE;
	alarm_counter++;
}

void reset_alarm_flag(){
  alarm_flag = FALSE;
}

void reset_alarm_counter() {
	alarm_counter = 0;
}

int main(int argc, unsigned char** argv){

  int fd;

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

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd <0) {perror(argv[2]); exit(-1); }
  llOpen(fd);

  unsigned char* start = (unsigned char*)malloc(sizeof(unsigned char));
  if(start == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }
	printf("waiting start\n");
  unsigned int startSize = readPacket(fd, start);
  start = changed;
	printf("received start\n");
  if(getFileInfo(start) == -1){
    printf("File Size and File Name not in the correct order, first size, then name\n");
    return -1;
  }
	printf("extracted info\n");
  info.content = (unsigned char*)malloc(info.size * sizeof(unsigned char));
  if(info.content == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }

	printf("[main] startSize: %i\n", startSize);

  readContent(fd, start, startSize);

  createFile();

  free(start);
  free(info.name);
  free(info.content);
  readControlMessage(fd, C_DISC);
  llClose(fd);
  return 0;
}

int llOpen(int fd){
  if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 1; /* unsigned inter-unsigned character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 unsigned chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  printf("New termios structure set\n");

  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  //Check conection
  readControlMessage(fd, C_SET);

  writeControlMessage(fd, C_UA);

  return 0;
}

int llClose(int fd){
  writeControlMessage(fd, C_DISC);

  readControlMessage(fd, C_UA);

  tcsetattr(fd, TCSANOW, &oldtio);

  close(fd);

  return 0;
}

void readControlMessage(int fd, unsigned char control){
    unsigned char buf[1];
    unsigned char message[5];
    unsigned int res = 0;
    unsigned int retry = TRUE;
    unsigned int complete = FALSE;
    unsigned int state = 0;
    unsigned int i = 0;

    while (retry == TRUE) {
      while (complete == FALSE) {
        res = read(fd,buf,1);
        if(res > 0){
          switch(state){
            case 0:
              if(buf[0] == FLAG){
                state++;
                message[i] = FLAG;
                i++;
              }
              break;
            case 1:
              if(buf[0] != FLAG){
                message[i] = buf[0];
                i++;
                if(i==4)
                  state++;
              } else {
                i = 0;
                state = 0;
              }
              break;
            case 2:
              if(buf[0] != FLAG){
                i = 0;
                state = 0;
              } else {
                message[i] = buf[0];
                complete = TRUE;
              }
              break;
          }
        }
      }

      if(message[0] == FLAG
          && message[1] == A
          && message[2] == control
          && message[3] == (A^control)
          && message[4] == FLAG)
      	retry = FALSE;
      else if(message[0] == FLAG
          && message[1] == A
          && message[2] == C_DISC
          && message[3] == (A^C_DISC)
          && message[4] == FLAG){
            llClose(fd);
            exit(0);
          } else {
            state = 0;
            complete = FALSE;
            i = 0;
          }
    }
}

void writeControlMessage(int fd, unsigned char control){
  unsigned char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = control;
  message[3] = A ^ control;
  message[4] = FLAG;
  write(fd, message, 5);
}

unsigned int llread(int fd, unsigned char* buffer){
  unsigned int stop = FALSE;
  unsigned int state = 0;
  unsigned char buf, c;
	unsigned int packetSize = 0;
  unsigned int res = 0;
  unsigned int disc = FALSE;

  packet = -1;

  while(!stop){
  	res = read(fd, &buf, 1);
    if(res > 0){
			//printf("unsigned char: 0x%02x \n", buf);
    	switch(state){
    		case 0: // start
					//printf("state 0\n");
    			if(buf == FLAG)
    				state++;
    			break;
    		case 1: // address
					//printf("state 1\n");
          disc = FALSE;
    			if(buf == A)
    				state++;
    			else
    				if(buf != FLAG)
    					state = 0;
    			break;
    		case 2: // control
					//printf("state 2\n");
          switch(buf){
            case C_0:
              packet = 0;
              c = buf;
        			state++;
              break;
            case C_1:
              packet = 1;
              c = buf;
          		state++;
              break;
            case C_DISC:
              c = buf;
              state++;
              disc = TRUE;
              break;
            case FLAG:
              state = 1;
              break;
            default:
              state = 0;
              break;
          }
    			break;
    		case 3: // bcc1
					//printf("state 3\n");
    			if (buf == (A ^ c)){
            if(disc){
              state = 5;
              disc = FALSE;
            } else
    				  state++;
    			} else
    				if(buf == FLAG)
    					state = 1;
    				else
    					state = 0;
    			break;
    		case 4: //data
					//printf("state 4\n");
    			if (buf == FLAG) {
    				stop = TRUE;
    			} else {
    				packetSize++;
    				buffer = (unsigned char *) realloc(buffer, packetSize * sizeof(unsigned char));
            if(buffer == NULL){
              printf("Tried to realloc, out of memory\n");
              exit(-1);
            }
    				*(buffer + packetSize - 1) = buf;
    			}
          break;
        case 5: //disc
          //printf("state 5\n");
          if (buf == FLAG) {
    				llClose(fd);
            exit(0);
    			} else
            state = 0;
          break;
    		}}
  }

  changed = buffer;

	return packetSize;
}

int destuffing(unsigned char* buffer, unsigned int packetSize){
	unsigned char buf, buf2;
	unsigned char * buffer2 = (unsigned char*)malloc(packetSize * sizeof(unsigned char));
  if(buffer2 == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }
	unsigned int newPacketSize = packetSize;
	unsigned int j = 0;

	memcpy(buffer2, buffer, packetSize);

	for(unsigned int i = 0; i < packetSize; i++){
		buf = *(buffer2 + i);
		if(buf == ESC){
			buf2 = *(buffer2 + i + 1);
			if(buf2 == ESC_FLAG){
				*(buffer + j) = FLAG;
			} else if(buf2 == ESC_ESC){
				*(buffer + j) = ESC;
			} else {
				return -1;
			}
			newPacketSize--;
			buffer = (unsigned char *) realloc(buffer, newPacketSize * sizeof(unsigned char));
      if(buffer == NULL){
        printf("Tried to realloc, out of memory\n");
        exit(-1);
      }
			i++;
		} else
			*(buffer + j) = buf;
		j++;
	}

  changed = buffer;

  free(buffer2);
	return newPacketSize;
}

int checkBCC2(unsigned char* buffer, unsigned int packetSize){
  unsigned char bcc2 = *(buffer + packetSize - 1);
  unsigned char track = *buffer;

  for(unsigned int i = 1; i < packetSize-1; i++){
    track ^= *(buffer + i);
	}

  packetSize--;
  buffer = (unsigned char*) realloc(buffer, packetSize * sizeof(unsigned char));
  if(buffer == NULL){
    printf("Tried to realloc, out of memory\n");
    exit(-1);
  }

  if(track == bcc2)
    return packetSize;

  changed = buffer;

  return -1;
}

unsigned int readPacket(int fd, unsigned char* buffer){
  int packetSize;

  do {
    packetSize = llread(fd, buffer);
    buffer = changed;
		printf("got it\n");
  	packetSize = destuffing(buffer, packetSize);
    buffer = changed;
		printf("destuffed\n");
    packetSize = checkBCC2(buffer, packetSize);
    buffer = changed;
		printf("checked bcc2\n");
    if(packetSize != -1){
      if(packet == 0)
        writeControlMessage(fd, RR_C_1);
      else
        writeControlMessage(fd, RR_C_0);

      if (packet == expected)
        expected ^= 1;
      else
        packetSize = -1;
    } else {
      if(packet == 0) {
				printf("[readPacket] Rejected packet\n");
        writeControlMessage(fd, REJ_C_1);
      } else {
				printf("[readPacket] Rejected packet\n");
        writeControlMessage(fd, REJ_C_0);
			}
    }
  } while(packetSize == -1);

	printf("[readPacket] Packet read:\n");	

	for (int i = 0; i < packetSize; i++) {
		printf("[readPacket] 0x%02x\n", buffer[i]);
	}

  return packetSize;
}

int getFileInfo(unsigned char* start){
  unsigned char type = *(start);

  if(type != START)
    return -1;

  unsigned char param = *(start + 1);
  unsigned int octets = (unsigned int)*(start + 2);
  off_t octetVal;
  off_t size = 0;

  if(param != SIZE)
    return -1;

  for(unsigned int i = 0; i < octets; i++) {
    octetVal = (*(start + 3 + i) << ((octets-i-1) * 8));
    size = size | octetVal;
  }

  info.size = (unsigned int)size;

  unsigned char* next = start + 3 + octets;
  param = *(next);

  if(param != NAME)
    return -1;

  octets = (unsigned int)*(next + 1);

	printf("[getFileInfo] File name size: %i\n", (int) octets);

  unsigned char* name = (unsigned char*)malloc((octets+1) * sizeof(unsigned char));
  if(name == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }

	int i;
  for(i = 0; i < octets; i++) {
    *(name + i) = *(next + 2 + i);
  }

	*(name + i) = '\0';

	printf("[getFileInfo] File name: %s\n", name);

  info.name = name;

  return 0;
}

unsigned int isEndPacket(unsigned char* start, unsigned int startSize, unsigned char* end, unsigned int endSize) {

  unsigned char type = *(end);

	//printf("[isEndPacket] Start packet size: %i\n", startSize);
	//printf("[isEndPacket] End packet size: %i\n", endSize);

  if(startSize != endSize || type != END)
    return FALSE;

  for(unsigned int i = 1; i < startSize; i++) {
		//printf("[isEndPacket] Start byte in position %i: 0x%02x\n", i, *(start + i));
		//printf("[isEndPacket] End byte in position %i: 0x%02x\n", i, *(end + i));
    if (*(start + i) != *(end + i))
      return FALSE;
  }

  return TRUE;

}

unsigned int removeHeader(unsigned char* packet, unsigned int size){
  unsigned int newSize = size - 4;
  unsigned char *newPacket = (unsigned char*)malloc(newSize * sizeof(unsigned char));

	printf("[removeHeader] size: %i\n", size);

	printf("[removeHeader] packet:\n");

	for (int i = 0; i < size; i++) {
		printf("[removeHeader] 0x%02x\n", packet[i]);
	}

	printf("[removeHeader] Past malloc\n");

  if(newPacket == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }

  for (unsigned int i = 0; i < size; i++)
  {
    *(newPacket + i) = *(packet + i + 4);
  }

		printf("[removeHeader] Past read cycle\n");

  changed = newPacket;

  free(packet);

	printf("[removeHeader] Past free\n");

  return newSize;
}

void readContent(int fd, unsigned char* start, unsigned int startSize){
  unsigned char* packet;
  unsigned int packetSize;
  unsigned int index;

	reset_alarm_flag();
	reset_alarm_counter();
	message_received = FALSE;

  while(TRUE) {

    packet = (unsigned char*)malloc(sizeof(unsigned char));
    if(packet == NULL){
      printf("Tried to malloc, out of memory\n");
      exit(-1);
    }
    packetSize = readPacket(fd, packet);
    packet = changed;

		printf("[readContent] Calling isEndPacket\n");

    if(isEndPacket(start, startSize, packet, packetSize)){
      break;
    }

		printf("[readContent] Past isEndPacket\n");

    packetSize = removeHeader(packet, packetSize);
    packet = changed;

		printf("[readContent] Past removeHeader\n");		

    memcpy(info.content + index, packet, packetSize);
    index += packetSize;

    free(packet);
  }
}

void createFile(){
  FILE* file = fopen((unsigned char*)info.name, "wb+");
  fwrite((unsigned char*)info.content, sizeof(unsigned char), info.size, file);
  printf("File created\n");
  fclose(file);
}
