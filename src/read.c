/*Non-Canonical Input Processing*/
#include "read.h"

struct termios oldtio, newtio;

int main(int argc, char** argv) {
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
  if (fd <0) {perror(argv[1]); exit(-1); }

  if(llOpen(fd) == -1){
    printf("Failed to connect, timed out\n");
    return -1;
  }

  //cenas

  if(llClose(fd) == -1){
    printf("Failed to disconnect, timed out\n");
    return -1;
  }

  sleep(1);

  close(fd);
  return 0;
}

int llOpen() {
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
  if (readControlMessage(fd, C_SET) == -1)
    return -1;
  else
    sendControlMessage(fd, C_UA);

  return 0;
}

int llClose() {
  if (readControlMessage(fd, DISC_C) == -1)
    return -1;

  sendControlMessage(fd, DISC_C);

  if (readControlMessage(fd, UA_C) == -1)
    return -1;

  tcsetattr(fd, TCSANOW, &oldtio);

  return 0;
}

int readControlMessage(int fd, unsigned char control) {
    unsigned char buf;
    int retry = TRUE;
    int complete = FALSE;
    int state = 0;
    int i = 0;

    while (retry = TRUE){
      while (complete == FALSE) {
        res = read(fd,buf,1);
        if(res > 0){
          switch(state){
            case 0:
              if(buf == FLAG){
                state++;
                message[i] = FLAG;
                i++;
              }
              break;
            case 1:
              if(buf != FLAG){
                message[i] = buf;
                i++;
                if(i==4)
                  state++;
              } else {
                i = 0;
                state = 0;
              }
              break;
            case 2:
              if(buf != FLAG){
                i = 0;
                state = 0;
              } else {
                message[i] = buf;
                complete = TRUE;
              }
              break;
          }
        }
      }

      if(message[0] == FLAG && message[1] == A && message[2] == control && message[3] == A^control && message[4] == FLAG)
      	retry = FALSE;
      else {
        int state = 0;
        int complete = FALSE;
        int i = 0;
      }
    }

    return 0;
}

void sendControlMessage(int fd, unsigned char C) {
  unsigned char message[5];
  message[0] = FLAG;
  message[1] = A;
  message[2] = C;
  message[3] = A ^ C;
  message[4] = FLAG;
  write(fd, message, 5);
}
