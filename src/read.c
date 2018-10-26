/*Non-Canonical Input Processing*/
#include "read.h"

struct termios oldtio, newtio;
int packet;
int expected = 0;
FileInfo info;

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

  unsigned char* start = (unsigned char*)malloc(sizeof(unsigned char));
  int packetSize = readMessage(start);
  if(getFileInfo(start) == -1){
    printf("File Size and File Name not in the correct order, first size, then name\n");
    return -1;
  }
  info.content = (unsigned char*)malloc(info.size * sizeof(unsigned char));

  // read das tramas de informação a espera da trama de end

  //criação do ficheiro

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

void readControlMessage(int fd, unsigned char control){
    unsigned char buf[1];
    unsigned char message[5];
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

void writeControlMessage(int fd, unsigned char control){
  unsigned char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = control;
  message[3] = A ^ control;
  message[4] = FLAG;
  write(fd, message, 5);
}

int llread(int fd, unsigned char* buffer){
  int stop = FALSE;
  int state = 0;
  unsigned char buf, c;
	int packetSize = 0;
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
    				//bcc2 = *(buffer + packetSize - 1);
    				//buffer = (unsigned char *) realloc(buffer, packetSize - 1);
    				stop = TRUE;
    			} else {
    				packetSize++;
    				buffer = (unsigned char *) realloc(buffer, packetSize * sizeof(unsigned char));
    				*(buffer + packetSize - 1) = buf;
    				printf("contents in buffer: %s", buffer + packetSize - 1);
    			}
    		}
  }

	return packetSize;
}

int destuffing(unsigned char* buffer, int packetSize){
	unsigned char buf, buf2;
	unsigned char * buffer2 = (unsigned char*)malloc(packetSize * sizeof(unsigned char));
	int newPacketSize = packetSize;
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
			buffer = (unsigned char *) realloc(buffer, newPacketSize * sizeof(unsigned char));
			i++;
		} else
			*(buffer + j) = buf;
		j++;
	}

  free(buffer2);
	return newPacketSize;
}

int checkBCC2(unsigned char* buffer, int packetSize){
  unsigned char bcc2 = *(buffer + packetSize - 1);
  unsigned char track = *buffer;

  for(int i = 1; i < packetSize-1; i++){
    track ^= *(buffer + i);
	}

  packetSize--;
  buffer = (unsigned char*) realloc(buffer, packetSize * sizeof(unsigned char));

  if(track == bcc2)
    return packetSize;

  return -1;
}

int readMessage(unsigned char* buffer){
  int packetSize;

  do {
    packetSize = llread(fd, buffer);
  	packetSize = destuffing(buffer, packetSize);
    packetSize = checkBCC2(buffer, packetSize);
    if(packetSize != -1){
      if(packet == 0)
        sendControlMessage(fd, RR_C_1);
      else
        sendControlMessage(fd, RR_C_0);

      if (packet == expected)
        expected ^= 1;
      else
        packetSize = -1;
    } else {
      if(packet == 0)
        sendControlMessage(fd, REJ_C_1);
      else
        sendControlMessage(fd, REJ_C_0);
    }
  } while(packetSize == -1)

  return packetSize;
}

int getFileInfo(unsigned char* start){
  int type = (int)*(start);

  if(type != 2)
    return -1;

  int param = (int)*(start + 1);
  int octets = (int)*(start + 2);
  int octetVal;
  int size = 0;

  if(param != 0)
    return -1;

  for (int i = 0; i < octets; i++) {
    octetVal = (int)*(start + 3 + i);
    for (int j = 0; j < (octets - i - 1); j++) {
      octetVal *= 256;
    }
    size += octetVal;
  }

  info.size = size;

  unsigned char* next = start + 3 + octets;
  param = (int)*(next);

  if(param != 1)
    return -1;

  octets = (int)*(next + 1);
  unsigned char* name =  (unsigned char*)malloc(octets * sizeof(unsigned char));

  for (int i = 0; i < octets; i++) {
    *(name + i) = *(next + 2 + i);
  }

  info.name = name;

  return 0;
}
