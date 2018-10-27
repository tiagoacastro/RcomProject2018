/*Non-Canonical Input Processing*/
#include "receiver.h"

struct termios oldtio, newtio;
int packet;
int expected = 0;
struct FileInfo info;
char* changed;

int main(int argc, char** argv){

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

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[2]); exit(-1); }
  llOpen(fd);

  char* start = (char*)malloc(sizeof(char));
  if(start == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }
  unsigned int startSize = readPacket(fd, start);
  start = changed;
  if(getFileInfo(start) == -1){
    printf("File Size and File Name not in the correct order, first size, then name\n");
    return -1;
  }
  info.content = (char*)malloc(info.size * sizeof(char));
  if(info.content == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }

  readContent(fd, start, startSize);

  createFile();

  free(start);
  free(info.name);
  free(info.content);
  llClose(fd);
  close(fd);
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

  newtio.c_cc[VTIME] = 1; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

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
  readControlMessage(fd, C_DISC);

  writeControlMessage(fd, C_DISC);

  readControlMessage(fd, C_UA);

  tcsetattr(fd, TCSANOW, &oldtio);

  return 0;
}

void readControlMessage(int fd, char control){
    char buf[1];
    char message[5];
    int res = 0;
    int retry = TRUE;
    int complete = FALSE;
    int state = 0;
    int i = 0;

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
      else {
        state = 0;
        complete = FALSE;
        i = 0;
      }
    }
}

void writeControlMessage(int fd, char control){
  char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = control;
  message[3] = A ^ control;
  message[4] = FLAG;
  write(fd, message, 5);
}

int llread(int fd, char* buffer){
  int stop = FALSE;
  int state = 0;
  char buf, c;
	unsigned int packetSize = 0;
  int res = 0;

  packet = -1;

  while(!stop){
  	res = read(fd, &buf, 1);
    if(res > 0)
    	switch(state){
    		case 0: // start
    			if(buf == FLAG)
    				state++;
    			break;
    		case 1: // address
    			if(buf == A)
    				state++;
    			else
    				if(buf != FLAG)
    					state = 0;
    			break;
    		case 2: // control
    			if(buf == C_0)
    				packet = 0;
    			else
    				if(buf == C_1)
    					packet = 1;
    				else{
    					if(buf == FLAG)
    						state = 1;
    					else
    						state = 0;
    					break;
    				}
    			c = buf;
    			state++;
    			break;
    		case 3: // bcc1
    			if (buf == (A ^ c))
    				state++;
    			else
    				if(buf == FLAG)
    					state = 1;
    				else
    					state = 0;
    			break;
    		case 4: //data
    			if (buf == FLAG) {
    				stop = TRUE;
    			} else {
    				packetSize++;
    				buffer = (char *) realloc(buffer, packetSize * sizeof(char));
            if(buffer == NULL){
              printf("Tried to realloc, out of memory\n");
              exit(-1);
            }
    				*(buffer + packetSize - 1) = buf;
    			}
    		}
  }

  changed = buffer;

	return packetSize;
}

int destuffing(char* buffer, unsigned int packetSize){
	char buf, buf2;
	char * buffer2 = (char*)malloc(packetSize * sizeof(char));
  if(buffer2 == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }
	unsigned int newPacketSize = packetSize;
	int j = 0;

	memcpy(buffer2, buffer, packetSize);

	for(int i = 0; i < packetSize; i++){
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
			buffer = (char *) realloc(buffer, newPacketSize * sizeof(char));
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

int checkBCC2(char* buffer, unsigned int packetSize){
  char bcc2 = *(buffer + packetSize - 1);
  char track = *buffer;

  for(int i = 1; i < packetSize-1; i++){
    track ^= *(buffer + i);
	}

  packetSize--;
  buffer = (char*) realloc(buffer, packetSize * sizeof(char));
  if(buffer == NULL){
    printf("Tried to realloc, out of memory\n");
    exit(-1);
  }

  if(track == bcc2)
    return packetSize;

  changed = buffer;

  return -1;
}

int readPacket(int fd, char* buffer){
  unsigned int packetSize;

  do {
    packetSize = llread(fd, buffer);
    buffer = changed;
  	packetSize = destuffing(buffer, packetSize);
    buffer = changed;
    packetSize = checkBCC2(buffer, packetSize);
    buffer = changed;
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
      if(packet == 0)
        writeControlMessage(fd, REJ_C_1);
      else
        writeControlMessage(fd, REJ_C_0);
    }
  } while(packetSize == -1);

  return packetSize;
}

int getFileInfo(char* start){
  int type = *(start);

  if(type != START)
    return -1;

  int param = (int)*(start + 1);
  int octets = (int)*(start + 2);
  unsigned int octetVal;
  unsigned int size = 0;

  if(param != SIZE)
    return -1;

  for(int i = 0; i < octets; i++) {
    octetVal = (unsigned int)*(start + 3 + i);
    for (int j = 0; j < (octets - i - 1); j++) {
      octetVal *= 256;
    }
    size += octetVal;
  }

  info.size = size;

  char* next = start + 3 + octets;
  param = (int)*(next);

  if(param != NAME)
    return -1;

  octets = (int)*(next + 1);
  char* name =  (char*)malloc(octets * sizeof(char));
  if(name == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }

  for(int i = 0; i < octets; i++) {
    *(name + i) = *(next + 2 + i);
  }

  info.name = name;

  return 0;
}

int isEndPacket(char* start, int startSize, char* end, int endSize){
  int type = *(end);
  if(startSize != endSize || type != END)
    return FALSE;

  for(int i = 1; i < startSize; i++){
    if (*(start + i) != *(end + i))
      return FALSE;
  }

  return TRUE;
}

int removeHeader(char* packet, int size){
  int newSize = size - 4;
  char *newPacket = (char*)malloc(newSize * sizeof(char));
  if(newPacket == NULL){
    printf("Tried to malloc, out of memory\n");
    exit(-1);
  }

  for (int i = 0; i < size; i++)
  {
    *(newPacket + i) = *(packet + i + 4);
  }

  changed = newPacket;

  free(packet);
  return newSize;
}

void readContent(int fd, char* start, unsigned int startSize){
  char* packet;
  unsigned int packetSize;
  unsigned int index;

  while(TRUE){
    packet = (char*)malloc(sizeof(char));
    if(packet == NULL){
      printf("Tried to malloc, out of memory\n");
      exit(-1);
    }
    packetSize = readPacket(fd, packet);
    packet = changed;

    if(isEndPacket(start, startSize, packet, packetSize)){
      break;
    }

    packetSize = removeHeader(packet, packetSize);
    packet = changed;

    memcpy(info.content + index, packet, packetSize);
    index += packetSize;

    free(packet);
  }
}

void createFile(){
  FILE* file = fopen((char*)info.name, "wb+");
  fwrite((char*)info.content, sizeof(char), info.size, file);
  printf("File created\n");
  fclose(file);
}
