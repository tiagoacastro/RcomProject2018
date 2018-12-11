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

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"

int main(int argc, char** argv) {

  /*
  * TCP
  */

	int	sockfd;
	struct	sockaddr_in server_addr;
	char	buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";  
	int	bytes;
	
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

	close(sockfd);
  return 0;
}

void parseArguments(char * user, char * pass, char * host, char * url) {

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

void getIp(char * address) {

  struct hostent *h;             
  if ((h=gethostbyname(address)) == NULL) {  
    herror("gethostbyname");
    exit(1);
  }

  printf("Host name  : %s\n", h->h_name);
  printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

  return 0;
}

int sendCommand(int socketfd) {
  char buf[] = "Mensagem de teste na travessia da pilha TCP/IP\n";
  bytes = write(sockfd, buf, strlen(buf));
	printf("Bytes escritos %d\n", bytes);
  return bytes;
}

void readResponse() {

}


