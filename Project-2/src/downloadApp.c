#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <strings.h>
 
#define SERVER_PORT          21
#define SERVER_ADDR          "192.168.28.96"
#define MAX_STRING_LENGTH    64
 
void parseInfo(char* info, char* user, char* password, char* host, char* path);
void parseFile(char* path, char* file);
int sendMsg(int sockfd, char* toSend);
int login(int sockfd, char* user, char* pass);
int download(int controlSockfd, int downloadSockfd, char * path, char * filename); 
int socketRead(int sockfd, char * reply);
 
int main(int argc, char** argv) {
  int  sockfd, sockfd_download;
  struct  sockaddr_in server_addr, server_addr_download;
  struct hostent * h;
 
  char user[MAX_STRING_LENGTH];
  char password[MAX_STRING_LENGTH];
  char host[MAX_STRING_LENGTH];
  char path[MAX_STRING_LENGTH];
 
  if (argc != 2) {
    printf("Wrong num of arguments\n");
    exit(1);
  }
 
  //parse argument
  parseInfo(argv[1], user, password, host, path);
 
  printf(">Username: %s\n", user);
  printf(">Password: %s\n\n", password);
  printf(">Host: %s\n", host);
  printf(">Path: %s\n", path);

  char file[MAX_STRING_LENGTH];
 
  parseFile(path, file);
 
  printf(">Filename: %s\n\n", file);
 
  //get host info
  if ((h=gethostbyname(host)) == NULL) {  
    herror("gethostbyname");
    exit(1);
    }
 
  printf(">Host name  : %s\n", h->h_name);
  printf(">IP Address : %s\n\n",inet_ntoa(*((struct in_addr *)h->h_addr)));
 
  //server address handling
  bzero((char*)&server_addr,sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr)));  /*32 bit Internet address network byte ordered*/
  server_addr.sin_port = htons(SERVER_PORT);    /*server TCP port must be network byte ordered */
    
  //open an TCP socket
  if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    perror("socket()");
    exit(0); 
  }
 
  //connect to the server
  if(connect(sockfd, 
            (struct sockaddr *)&server_addr, 
             sizeof(server_addr)) < 0){
    perror("connect()");
    exit(0);
  }
  
  //check connection
  if (socketRead(sockfd, NULL) != 2) {
    printf("Error establishing connection\n");
    exit(1);
  }

  //login
  if (login(sockfd, user, password)) {
    printf("Error login\n");
    exit(1);
  }
 
  //passive mode
  sendMsg(sockfd, "PASV\n");
  int serverPort;
  char passive[MAX_STRING_LENGTH];
  socketRead(sockfd, passive);

  //227 Entering Passive Mode (ip1, ip2, ip3, ip4, p1, p2)
  int ip1, ip2, ip3, ip4, port1, port2;
  if ((sscanf(passive, "227 Entering Passive Mode (%d, %d, %d, %d, %d, %d)", &ip1, &ip2, &ip3, &ip4, &port1, &port2)) < 0){
      printf("Error entering passive mode\n");
      exit(1);
  }

  if ((sprintf(passive, "%d.%d.%d.%d", ip1, ip2, ip3, ip4))< 0) {
		printf("Error creating ip address.\n");
		return 1;
	}

  serverPort = port1 * 256 + port2;

  printf(">Passive IP: %s\n", passive);
  printf(">Passive PORT: %d\n", serverPort);
 
  //open port
  bzero((char*)&server_addr_download,sizeof(server_addr_download));
  server_addr_download.sin_family = AF_INET;
  server_addr_download.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr)));  
  server_addr_download.sin_port = htons(serverPort);    

  if ((sockfd_download = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    perror("socket()");
    exit(0); 
  }
 
  if(connect(sockfd_download, (struct sockaddr *)&server_addr_download, sizeof(server_addr_download)) < 0){
    perror("connect()");
    exit(0);
  }
 
  //download file
  download(sockfd, sockfd_download, path, file);
 
  //close sockets
  close(sockfd_download);
  close(sockfd);
 
  return 0;
}
 
void parseInfo(char* info, char* user, char* password, char* host, char* path) {
  unsigned int i = 6;
  unsigned int j = 0;
  unsigned int length = strlen(info);
  unsigned int state = 0;
  unsigned char buf;

  while(i<length){
    buf = *(info+i);
 
    switch(state){
      case 0:
        if(buf == ':'){
          *(user + j) = 0; //end char
          state++;
          j = 0;
        }else{
          if(buf == '/'){
            *(user + j) = 0; //end char
            j=0;
            memcpy(host, user, MAX_STRING_LENGTH);
            strcpy(user, "anonymous");
            strcpy(password, "ds");
            state = 3;
          }else{
            *(user + j) = buf;
            j++;
          }
        }
        break;
      case 1:
        if(buf == '@'){
          *(password + j) = 0; //end char
          state++;
          j = 0;
        }else{
          *(password + j) = buf;
          j++;
        }
        break;
      case 2:
        if(buf == '/'){
          *(host + j) = 0; //end char
          state++;
          j = 0;
        }else{
          *(host + j) = buf;
          j++;
        }
        break;
      case 3:
        *(path + j) = buf;
        j++;
        break;
    }

	*(path + j) = 0; //end char
 
    i++;
  }
}
 
void parseFile(char* path, char* file) {
  unsigned int length = strlen(path);
  unsigned int i = 0;
  unsigned int j = 0;
 
  while(i <= length){
    if(*(path + i) == 0){
      *(file + j) = 0; //end char;
      break;
    }else{
      if(*(path + i) == '/'){
        j = 0;
      }else{
        *(file + j) = *(path + i);
        j++;
      }
    }
    i++;
  }
}	

int socketRead(int sockfd, char* reply){
  FILE* fp = fdopen(sockfd, "r");
  int allocated = 0;

  if(reply == NULL){
    reply = (char*) malloc(sizeof(char) * MAX_STRING_LENGTH);
    allocated = 1;
  }

  do {
    memset(reply, 0, MAX_STRING_LENGTH);
    reply = fgets(reply, MAX_STRING_LENGTH, fp);
  } while (!('1' <= reply[0] && reply[0] <= '5') || reply[3] != ' ');

  int returnValue;
  char code[2] = {reply[0], '\0'};
  sscanf(code, "%d", &returnValue);

  if(allocated)
    free(reply);

	return returnValue;

}
 
int sendMsg(int sockfd, char* toSend) {
  int bytes;
  bytes = write(sockfd, toSend, strlen(toSend));
  return bytes;
}

int login(int sockfd, char* user, char* pass) {
  char userCmd[MAX_STRING_LENGTH];
  char passCmd[MAX_STRING_LENGTH];
 
  sprintf(userCmd, "USER %s\n", user); // \r ???
  sendMsg(sockfd, userCmd);
  int userReply = socketRead(sockfd, NULL);
  if (userReply >= 4) {
    printf("Error sending username\n");
    return 1;
  }
 
  sprintf(passCmd, "PASS %s\n", pass); //same thing ??? 
  sendMsg(sockfd, passCmd);
  int passReply = socketRead(sockfd, NULL);
  if(passReply >= 4) {
    printf("Error sending password\n");
    return 1;
  }
  
  return 0;
}

int download(int controlSockfd, int downloadSockfd, char * path, char * filename) {
	FILE * file;
	int bytes;
  char retrCmd[MAX_STRING_LENGTH];
 
  sprintf(retrCmd, "RETR %s\n", path); 
  sendMsg(controlSockfd, retrCmd);
  int retrReply = socketRead(controlSockfd, NULL);
  if (retrReply >= 4) {
    printf("Error sending retrieve command\n");
    return 1;
  }

	if (!(file = fopen(filename, "w"))) {
		printf("Cannot open file.\n");
		return 1;
	}

	char buf[MAX_STRING_LENGTH];
	while ((bytes = read(downloadSockfd, buf, sizeof(buf)))) {
		if (bytes < 0) {
			printf("No data received from socket\n");
			return 1;
		}

		if ((bytes = fwrite(buf, bytes, 1, file)) < 0) {
			printf("Cannot write data in file\n");
			return 1;
		}
	}

	fclose(file);
	return 0;
}
