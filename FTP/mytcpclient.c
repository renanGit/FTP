/*
* Renan Santana / PID: 4031451 / HW1 / DUE: OCT 14, 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//
#include <dirent.h>
//#include <sys/stat.h>	// change write permission
#include <errno.h>
//

void syserr(char* msg) { perror(msg); exit(-1); }

char* requestData(char* request, int sockfd){
	int n;
	
	n = send(sockfd, request, 255, 0);
	if(n < 0) { syserr("Err 1.1: Can't Send To Server"); }

	n = recv(sockfd, request, 255, 0);
	if(n < 0){ syserr("Err 1.2: Can't Receive From Server"); }
	return request;
}

void lslocal(){
	int n;
	DIR *dir;
	struct dirent *elm;
	
	if((dir = opendir("./ClientFiles")) != NULL){
		while((elm = readdir(dir)) != NULL){ printf("%s\n", elm->d_name); }
		closedir(dir);
	}
	else{ perror("Can't open directory: "); }
}

void lsremote(int sockfd, char token1[]){
	int n;
	char buffer[128];
	// get approval from server
	char* fileName = requestData(token1, sockfd);
	
	if(fileName[0] == '\0'){ printf("No File Existance In Server\n"); }
	else{
		// receive the list of file one be one
		while((n = recv(sockfd, buffer, 127, 0)) > 0){
			if(buffer[0] == '\0'){ break; }
			buffer[n] = '\0';
			printf("%s\n", buffer);
		}
		if(n < 0) { syserr("Err 2.1: Can't Receive From Server"); }
	}
}

void put(int sockfd, char buffer2[], char token2[]){
	int n, size, fileSize;
	char tempVar[128];
	char fileName[128];
	char cat[128];
	unsigned char buffer[1024];
	unsigned short int found = 0;
	FILE *fp;
	DIR *dir;
	struct dirent *elm;
	
	requestData(buffer2, sockfd);
	
	if((dir = opendir("./ClientFiles")) != NULL){
	
		// send to the client directory is valid
		tempVar[0] = '0';
		n = send(sockfd, tempVar, 127, 0);
		if(n < 0) { syserr("Err 2.1: Can't Send To Server"); }
	
		while((elm = readdir(dir)) != NULL){
			strcpy(fileName, elm->d_name);
			if(strcmp(fileName, token2) == 0){ found = 1; break; }
		}
		closedir(dir);

		if(found){
			bzero(cat, 128);
			strcat(cat, "./ClientFiles/");
			strcat(cat, token2);
			
			fp = fopen(cat, "rb");
			if(fp == NULL){ perror("Bad File Name: "); exit(1); }
			
			//Get file length
			bzero(tempVar, 127);
			fseek(fp, 0, SEEK_END);
			fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			sprintf(tempVar, "%i", fileSize);
			
			// send to the client that we found the file
			n = send(sockfd, tempVar, 127, 0);
			if(n < 0) { syserr("Err 2.2: Can't Send To Server"); }
			
			do {
				size = fread(buffer, sizeof(unsigned char), sizeof(buffer), fp);
				if(size < sizeof(buffer)){
					n = send(sockfd, buffer, size, 0);
					if(n < 0) { syserr("Err 2.3: Can't Send To Server"); }
				}
				else{
					n = send(sockfd, buffer, 1024, 0);
					if(n < 0) { syserr("Err 2.3: Can't Send To Server"); }
				}
				bzero(buffer, 1023);
			} while (size == sizeof(buffer)); // if it was a full buffer, loop again
			
			printf("Upload '%s' to remote server: successful\n", token2);
			fclose(fp);
		}
		else{
			printf("File '%s' at client was not found.\n", token2);
			tempVar[0] = '\0';
			n = send(sockfd, tempVar, 127, 0);
			if(n < 0) { syserr("Err 2.5: Can't Send To Server"); }
		}
	}
	else{ 
		closedir(dir);
		printf("Can't open directory\n");
		tempVar[0] = '\0';
		n = send(sockfd, tempVar, 127, 0);
		if(n < 0) { syserr("Err 2.6: Can't Send To Server"); }	
	}
}

void get(int sockfd, char buffer2[], char token2[]){
	int n, fileSize, amountWrote;
	char cat[128];
	char buffer[256];
	unsigned char fileBuffer[1024];
	FILE *fp;
	
	// first check : make sure the directory is valid
	char* boolFile = requestData(buffer2, sockfd);

	if(boolFile[0] != '\0'){
		// second check : make sure the file exist
		n = recv(sockfd, buffer, 255, 0);
		if(n < 0) { syserr("Err 3.1: Can't Receive From Server"); }
		
		if(buffer[0] != '\0'){
			
			buffer[n] = '\0';
			fileSize = atoi(buffer);
			
			bzero(cat, 127);
			strcat(cat, "./ClientFiles/");
			strcat(cat, token2);
			
			//Open file "+" create file if not existing
			fp = fopen(cat, "w+b");
			if (fp == NULL){
				perror("FP NULL: "); 
				strerror(errno);
				exit(1);
			}
			
			do{
				if(fileSize < sizeof(fileBuffer)){
					n = recv(sockfd, fileBuffer, fileSize, 0);
					if(n < 0) { syserr("Err 3.2: Can't Receive From Server"); }
				}
				else{
					n = recv(sockfd, fileBuffer, 1024, 0);
					if(n < 0) { syserr("Err 3.2: Can't Receive From Server"); }
				}
				
				fwrite(fileBuffer, sizeof(unsigned char), n, fp);
				fileSize -= n;
				
				bzero(fileBuffer, 1024);
			}while(fileSize > 0);
			
			
			printf("Retrieve '%s' from remote server: successful\n", token2);
			fclose(fp);
		}
		else{ printf("File '%s' at remote server was not found.\n", token2); }
	}
	else{ printf("Directory in Server is invalid.\n"); }
}

int main(int argc, char* argv[]){
	int sockfd, portno, n;
	struct hostent* server;
	struct sockaddr_in serv_addr;
	char token1[32], token2[32], inputLine[32];
	char buffer[256];
	
	if(argc != 3) {
		if(argc == 2){
			portno = 5555;
		}
		else{
			fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
			return 1;
		}
	}
	else{ portno = atoi(argv[2]); }
	
	server = gethostbyname(argv[1]);
	if(!server) {
		fprintf(stderr, "ERROR: no such host: %s\n", argv[1]);
		return 2;
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd < 0) { syserr("can't open socket"); }

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
	serv_addr.sin_port = htons(portno);

	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		syserr("Can't connect to server.");
	
	printf("Connection to %s:%i established. Now awaiting commands...\n", argv[1], portno);
	snprintf(inputLine, sizeof(inputLine), "%s:%i>",argv[1], portno);
	
	while(1){
		
		printf("%s ",inputLine);
		fgets(buffer, 255, stdin);
		n = strlen(buffer);
		
		if(n > 0 && buffer[n-1] == '\n') { buffer[n-1] = '\0'; }
		
		// parse the string
		sscanf(buffer, "%s %s", token1, token2);
		
		// Fit the token in one of these commands
		if(strcmp("ls-local",token1) == 0){ lslocal(); }
		else if(strcmp("ls-remote",token1) == 0){ lsremote(sockfd, buffer); }
		else if(strcmp("put",token1) == 0){ put(sockfd, buffer, token2); }
		else if(strcmp("get",token1) == 0){ get(sockfd, buffer, token2); }
		else if(strcmp("exit",buffer) == 0){ requestData(buffer, sockfd); break; }
		else { puts("Input is not valid."); }
		
		memset(buffer, 0, sizeof(buffer));
		memset(token1, 0, sizeof(token1));
		memset(token2, 0, sizeof(token2));
	}
	
	printf("Connection to server %s terminated. Bye now!\n", inputLine);
	close(sockfd);
	return 0;
}
