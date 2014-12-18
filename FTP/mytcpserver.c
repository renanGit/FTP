/*
* Renan Santana / PID: 4031451 / HW1 / DUE: OCT 14, 2014
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
//
#include <dirent.h>		// dir file names
//#include <sys/stat.h>	// change write permission
#include <errno.h>
//

void syserr(char *msg) { perror(msg); exit(-1); }

void lsremote(int newsockfd){
	int n;
	char fileName[128];
	char temp[256];
	DIR *dir;
	struct dirent *elm;
	
	if((dir = opendir("./ServerFiles")) != NULL){
		temp[0] = '0';
		n = send(newsockfd, temp, 255, 0);
		if(n < 0) { syserr("Err 1.1: Can't Send To Client"); }
		
		while((elm = readdir(dir)) != NULL){
			bzero(fileName, 128);
			strcpy(fileName, elm->d_name);
			
			n = send(newsockfd, fileName, 127, 0);
			if(n < 0) { break; syserr("Err 1.2: Can't Send To Client"); }
		}
		
		closedir(dir);
		fileName[0] = '\0';
		n = send(newsockfd, fileName, 127, 0);
		if(n < 0) { syserr("Err 1.3: Can't Send To Client"); }
	}
	else{
		closedir(dir);
		perror("Can't open directory: ");
		temp[0] = '\0';
		n = send(newsockfd, temp, 255, 0);
		if(n < 0) { syserr("Err 1.4: Can't Send To Client"); }
	}
}

void put(int newsockfd, char token2[]){
	int n, fileSize, amountWrote;
	char tempVar[128];
	char cat[128];
	char buffer[256];
	unsigned char fileBuffer[1024];
	FILE *fp;
	
	bzero(fileBuffer, 1023);
	
	// send comfirmation
	buffer[0] = '0';
	n = send(newsockfd, buffer, 255, 0);
	if(n < 0) { syserr("Err 2.1: Can't Send to Client"); }
	
	// first check : make sure the directory is valid
	n = recv(newsockfd, tempVar, 127, 0);
	if(n < 0) { syserr("Err 2.2: Can't Receive From Client"); }
	
	if(tempVar[0] != '\0'){
		
		// second check : make sure the file exist
		n = recv(newsockfd, tempVar, 127, 0);
		if(n < 0) { syserr("Err 2.3: Can't Receive From Client"); }
		tempVar[n] = '\0';
		
		if(tempVar[0] != '\0'){
			
			fileSize = atoi(tempVar);
		
			bzero(cat, 127);
			strcat(cat, "./ServerFiles/");
			strcat(cat, token2);
			
			//Open file "+" create file if not existing
			fp = fopen(cat, "w+b");
			if (fp == NULL){
				fclose(fp); 
				perror("File Write Issue: "); 
				strerror(errno);
				return;
			}
			
			do{
				bzero(fileBuffer, 1024);
				if(fileSize < sizeof(fileBuffer)){
					n = recv(newsockfd, fileBuffer, fileSize, 0);
					if(n < 0) { syserr("Err 2.4: Can't Receive From Client"); }
				}
				else{
					n = recv(newsockfd, fileBuffer, 1024, 0);
					if(n < 0) { syserr("Err 2.4: Can't Receive From Client"); }
				}
				fwrite(fileBuffer, sizeof(unsigned char), n, fp);
				fileSize -= n;
				
			}while(fileSize > 0);
			
			puts("DOWNLOAD DONE");
			fclose(fp);
		}
		else{ printf("File was not found.\n"); }
	}
	else{ printf("Directory in Server is invalid.\n"); }
}

void get(int newsockfd, char token2[]){
	int n, size, fileSize;
	char tempVar[256];
	char fileName[128];
	char cat[128];
	unsigned char buffer[1024];
	unsigned short int found = 0;
	FILE *fp;
	DIR *dir;
	struct dirent *elm;
	
	if((dir = opendir("./ServerFiles")) != NULL){
		// send the client directory is valid
		tempVar[0] = '0';
		n = send(newsockfd, tempVar, 255, 0);
		if(n < 0) { syserr("Err 3.1: Can't Send To Client"); }
		
		while((elm = readdir(dir)) != NULL){
			strcpy(fileName, elm->d_name);
			if(strcmp(fileName, token2) == 0){ found = 1; break; }
		}
		closedir(dir);
		
		if(found){
		
			bzero(cat, 127);
			strcat(cat, "./ServerFiles/");
			strcat(cat, token2);
			
			fp = fopen(cat, "rb");
			if(fp == NULL){ perror("Bad File Name: "); return; }
			
			//Get file length
			bzero(tempVar, 255);
			fseek(fp, 0, SEEK_END);
			fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			sprintf(tempVar, "%i", fileSize);	// change int > str
			
			// send the client that we found the file
			n = send(newsockfd, tempVar, 255, 0);
			if(n < 0) { syserr("Err 3.2: Can't Send To Client"); }
			
			do {
				size = fread(buffer, sizeof(unsigned char), sizeof(buffer), fp);
				if(size < sizeof(buffer)){
					n = send(newsockfd, buffer, size, 0);
					if(n < 0) { syserr("Err 3.3: Can't Send To Client"); }
				}
				else{
					n = send(newsockfd, buffer, 1024, 0);
					if(n < 0) { syserr("Err 3.3: Can't Send To Client"); }
				}
				bzero(buffer, 1024);
			} while (size == sizeof(buffer)); // if it was a full buffer, loop again
			
			puts("UPLOAD DONE");
			fclose(fp);
		}
		else{
			printf("File was not found.\n");
			tempVar[0] = '\0';
			n = send(newsockfd, tempVar, 255, 0);
			if(n < 0) { syserr("Err 3.5: Can't Send To Client"); }
		}
	}
	else{ 
		closedir(dir);
		perror("Can't open directory: "); 
		tempVar[0] = '\0';
		n = send(newsockfd, tempVar, 255, 0);
		if(n < 0) { syserr("Err 3.6: Can't Send To Client"); }
	}
	
}

int main(int argc, char *argv[]){

	int sockfd, newsockfd, portno, n;
	struct sockaddr_in serv_addr, clt_addr;
	socklen_t addrlen;
	char buffer[256];
	pid_t  pid;

	if(argc != 2) { 
		if(argc == 1){
			portno = 5555;
		}
		else{
			fprintf(stderr,"Usage: %s <port#>\n", argv[0]);
			return 1;
		}
	}
	else
		portno = atoi(argv[1]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if(sockfd < 0){ syserr("Err 0: Can't Open Socket"); }
	printf("Create socket.\n");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
		syserr("Err 0: Can't Bind To Port.");
	printf("Bind socket to port#: %d.\n", portno);

	listen(sockfd, 5); 

	while(1) {
		
		printf("Waitting on port#: %d.\n", portno);
		addrlen = sizeof(clt_addr); 

		// accept() queues the connection / if no pending connections are present block
		newsockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &addrlen);
		if(newsockfd < 0){ syserr("Err 0: Can't Accept Connection."); }

		printf("New incoming connection, block on receive.\n");
		
		// fork the connection
		pid = fork();
		if (pid == 0) {
		
			char token1[32], token2[32];
			
			while(1){
				while((n = recv(newsockfd, buffer, 255, 0)) > 0){
					buffer[n] = '\0';
					
					printf("SERVER GOT MESSAGE: %s\n", buffer); 
					
					// parse the string
					sscanf(buffer, "%s %s", token1, token2);
					
					if(strcmp("ls-remote",token1) == 0){ lsremote(newsockfd); }
					else if(strcmp("put",token1) == 0){ put(newsockfd, token2); }
					else if (strcmp("get", token1) == 0){ get(newsockfd, token2); }
					else if(strcmp("exit", token1) == 0){
						printf("Kill child %i\n", getpid());
						
						n = send(newsockfd, buffer, 255, 0);
						if(n < 0) { syserr("Case 4: Can't Send To Client"); }
						
						close(newsockfd);
						kill(getpid(), SIGKILL);
					}

					memset(buffer, 0, sizeof(buffer));
					memset(token1, 0, sizeof(token1));
					memset(token2, 0, sizeof(token2));
				}
				if(n < 0) { syserr("Err 1: Can't Receive From Client"); }
				kill(getpid(), SIGKILL);
			}
		}// end child if
	}// end for
	
	close(sockfd); 
	return 0;
}
