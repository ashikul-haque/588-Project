#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <sys/stat.h>
#include <fcntl.h>
#include<pthread.h> //for threading , link with lpthread

#define SIZE 10240

void *connection_handler(void *);

void *trackerServer(void *);

int main(int argc , char *argv[])
{
	trackerServer(NULL);
	return 0;
}

/*
 * This will handle connection for each client
 * */

void *trackerServer(void *unused){
	int socket_desc , new_socket , c , *new_sock;
	struct sockaddr_in server , client;
	char *message;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		puts("Could not create socket");
		exit(1);
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 7658 );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		exit(1);
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");
		
		//Reply to the client
		//message = "Hello Client , I have received your connection. And now I will assign a handler for you\n";
		//write(new_socket , message , strlen(message));
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = new_socket;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("could not create thread");
			exit(1);
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");
	}
	
	if (new_socket<0)
	{
		perror("accept failed");
		exit(1);
	}
	exit(0);
	
} 

int getLine(FILE * fp, char *ip, char *port) {
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
	while((read = getline(&line, &len, fp)) != -1) {
		
		char ip_port[100];
		sprintf(ip_port,"%s %s\n",ip, port); 
		
		if(strcmp(ip_port, line) == 0) {
			return i;
		}
		bzero(line, read);
		i++;
	}
	fclose(fp);
	return 0;
}

void replaceLine(FILE * fp, char *fileName, int lineNo, char *start, char *end) {
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
    	printf("%d\n",lineNo);
    	FILE * nfp = fopen("temp.txt","w");
	while((read = getline(&line, &len, fp)) != -1) {
		puts(line);
		if(i==lineNo) {
			fprintf(nfp, "%s %s\n", start, end);
		}
		else {
			fprintf(nfp, "%s", line);
		}
		i++;
	}
	fclose(fp);
	fclose(nfp);
	remove(fileName);
    	rename("temp.txt",fileName);
}
 
void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char client_message[2000], my_message[2000];
	//message = "I am your handler\n";
	//write(sock , message , strlen(message));
	
	//Receive a message from client
	int i=0;
	char file_message[2000];
	while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
	{
		strcat(file_message, client_message);
		puts(client_message);
		//getting file name
		char *tempp;
		tempp = strtok(file_message, " ");
		if(strcmp(tempp, "create") ==0) {
			puts("create request");
			//getting file name
			tempp = strtok(NULL, " ");
			
			//creating tracker file
			char *token;
			char temp[100];
			token = strtok(tempp, ".");
			sprintf(token, "%s.track",token);
			FILE *fp;
			
			//checking if tracker already exists or not
			if( access( token, F_OK ) == 0 ) {
				printf("File already exists\n");
    				sprintf(temp,"<createtracker ferr>");
    				
			} else {
    				fp = fopen(token,"w");
    				char tracker[2000];
    				strncpy(tracker,&client_message[7],2000);
				if ( fp )
   				{
   					//tokenize except command
   					char *tracker_tok = strtok(tracker, " ");
   					
   					//copy all tokens
   					char *fileName = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *fileSize = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *desc = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *md5 = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *ip = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *port = strdup(tracker_tok);
   					port = strtok(port, "\n");
   					port = strtok(port, "\r");
   					fprintf(fp,"%s %s %s %s\n",fileName,fileSize,md5,desc);
   					fprintf(fp,"%s %s\n0 %s\n",ip,port,fileSize);
   					fclose(fp);
   					FILE *listFP;
   					char *listName = "list";
   					if (access( token, F_OK ) == 0 ) {
						listFP = fopen(listName, "a");
						fprintf(listFP,"%s %s %s\n",fileName,fileSize,md5);
					} 
					else {
						listFP = fopen(listName, "w");
						fprintf(listFP,"%s %s %s\n",fileName,fileSize,md5);
					}
					fclose(listFP);
    				}
    				else{
    					printf("failed to open file\n");
    					sprintf(temp,"<createtracker fail>");
    				}
				
				sprintf(temp,"<createtracker succ>");
			}
			strcat(my_message,temp);
			write(sock , my_message , strlen(my_message));
			bzero(client_message, 2000);
			bzero(my_message, 2000);
			bzero(file_message, 2000);		
			
		}
		else if(strcmp(tempp, "update") ==0) {
			//need to update existing ip information
			puts("update request");
			//getting file name
			tempp = strtok(NULL, " ");
			char *token;
			char temp[100];
			token = strtok(tempp, ".");
			sprintf(token, "%s.track",token);
			FILE *fp;
			if( access( token, F_OK ) == 0 ) {
				fp = fopen(token,"a+");
				char tracker[2000];
    				strncpy(tracker,&client_message[7],2000);
    				if ( fp )
   				{
   					char *tracker_tok = strtok(tracker, " ");
   					tracker_tok = strtok(NULL, " ");
   					char *start_bytes = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *end_bytes = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *ip = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					char *port = strdup(tracker_tok);
   					port = strtok(port, "\n");
   					port = strtok(port, "\r");
   					
   					//already exisitng ip port check
   					int lineCount = getLine(fp, ip, port);
   					if (lineCount == 0) {
   						fp = fopen(token,"a");
   						fprintf(fp,"%s %s\n%s %s\n",ip,port, start_bytes,end_bytes);
   						fclose(fp);
   					}
   					else {
   						fp = fopen(token,"r");
   						replaceLine(fp, token, lineCount+1, start_bytes, end_bytes);
   					}		
   					
   					
   					
    				}
    				else{
    					printf("failed to open file\n");
    					sprintf(temp,"<updatetracker %s fail>",token);
    				}
				sprintf(temp,"<updatetracker %s succ>",token);
    				
			} else {
				printf("File does not exist\n");
				sprintf(temp,"<updatetracker %s ferr>",token);
			}
			strcat(my_message,temp);
			write(sock , my_message , strlen(my_message));
			bzero(client_message, 2000);
			bzero(my_message, 2000);
			bzero(file_message, 2000);
		}
		else if(strcmp(tempp, "req") ==0) {
			FILE *fp;
			char *filename = "list";
			fp = fopen(filename, "r");
			char data[2000];
			char *begin = "<REP LIST X>\n";
			strcat(my_message,begin);
			while(fgets(data, 2000, fp) != NULL) {
				strcat(my_message,data);
			}
			char *end = "<REP LIST END>\n";
			strcat(my_message,end);
			write(sock , my_message , strlen(my_message));
			puts("List sent!!");
			bzero(client_message, 2000);
			bzero(my_message, 2000);
			bzero(file_message, 2000);
		}
		else if(strcmp(tempp, "get") ==0) {
			puts("get from server");
			tempp = strtok(NULL, " ");
			//tempp = strtok(tempp, "\n");
			FILE *fp;
			sprintf(tempp, "%s",tempp);
			//puts(tempp);
			
			if( access( tempp, F_OK ) == 0 ) {
				fp = fopen(tempp, "r");
				//struct stat s;
				//strcat(my_message,s.st_size);
				//write(sock , my_message , strlen(my_message));
				//printf("File size %ld sent!!\n",s.st_size);
  				char data[SIZE];
  				int i = 0;
  				bzero(data, SIZE);
  				int bs;
  				
  				while(!feof(fp)) {
  					bs = fread(data, 1, sizeof(data), fp);
					write(sock, data, bs);
    					bzero(data, SIZE);
    					i+=SIZE;	
  				}
  				
  				printf("\n");
  				char end[50];
  				sprintf(end,"%d bytes sent",i);
  				puts(end);
  				close(sock);
			} else{
				printf("failed to open file\n");
				close(sock);	
			}
			/*bzero(client_message, 2000);
			bzero(my_message, 2000);
			bzero(file_message, 2000);*/
		}
		
	}
	
	if(read_size == 0)
	{
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("Connection terminated");
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}
