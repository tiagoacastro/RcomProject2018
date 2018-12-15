#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <strings.h>
#include <string.h>

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"
#define MAX_LINE_LENGTH	 1024

int openTCPPort(struct sockaddr_in * server_addr);
struct hostent * getIp(char* address);
void parseInfo(char* info, char* user, char* password, char* host, char* path);
void parseFile(char* path, char* file);
int readCmdReply(int socketfd, char * reply); 
