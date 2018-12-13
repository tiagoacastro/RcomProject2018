#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>

#define SERVER_PORT 				21
#define SERVER_ADDR 				"192.168.28.96"
#define MAX_STRING_LENGTH		64

int main(int argc, char** argv) {
	int	sockfd;
	struct	sockaddr_in server_addr;
	struct hostent *h;

	char user[MAX_STRING_LENGTH];
	char password[MAX_STRING_LENGTH];
	char host[MAX_STRING_LENGTH];
	char path[MAX_STRING_LENGTH];

	parseInfo(argv[1], user, password, host, path);

	char file[MAX_STRING_LENGTH];

	parseFile(path, file);

	h = getip(host);

	sockfd = openTCPPort(server_addr);

	//continue here

	close(sockfd);
  return 0;
}

int openTCPPort(struct	sockaddr_in server_addr) {
  /*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */
    
	/*open a TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
  }

	/*connect to the server*/
  if(connect(sockfd, 
            (struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) {
    perror("connect()");
    exit(0);
	}

  return sockfd;
}

void getIp(char* address) {

  struct hostent* h;             
  if ((h=gethostbyname(address)) == NULL) {  
    herror("gethostbyname");
    exit(1);
  }

  printf("Host name  : %s\n", h->h_name);
  printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

  return h;
}

void parseInfo(char* info, char* user, char* password, char* host, char* path){
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
					*(user + j) = buf;
					j++;
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

void parseFile(char* path, char* file){
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
