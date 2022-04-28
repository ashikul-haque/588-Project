#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include<pthread.h> //for threading , link with lpthread

#define SIZE 1024
#define MESSAGE_SIZE 512
#define CHAR_SIZE 128

pthread_mutex_t lock1,lock2,lock3,lock4,lock5, lock6;

void *connection_handler(void *);

void *trackerServer(void *);

int getLine(FILE * fp, char *ip, char *port);

void replaceLine(FILE * fp, char *fileName, int lineNo, char *start, char *end);

void replaceStatus(FILE * fp, char *fileName, int lineNo, int status);

static int parse_ext(const struct dirent *dir);

void peerStatusUpdate(char *ip, char *port, int status);



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
	//puts("bind done");
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept and incoming connection
	//puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		//puts("Connection accepted");
		
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
		//puts("Handler assigned");
	}
	
	if (new_socket<0)
	{
		perror("accept failed");
		exit(1);
	}
	exit(0);
	
}

void *connection_handler(void *socket_desc)
{
	//Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	int peerId;
	char client_message[MESSAGE_SIZE], my_message[MESSAGE_SIZE], file_message[MESSAGE_SIZE];
	
	bzero(client_message, MESSAGE_SIZE);
	bzero(my_message, MESSAGE_SIZE);
	bzero(file_message, MESSAGE_SIZE);
	
	while( (read_size = recv(sock , client_message , MESSAGE_SIZE , 0)) > 0 )
	{
		strcat(file_message, client_message);

		//getting command by tokenizing
		char *tempp;
		tempp = strtok(file_message, " ");
		
		//intro message from peer containing its id, ip, port
		if(strcmp(tempp, "intro") == 0) {
		
			//get peer id from intro message
			tempp = strtok(NULL, " ");
			peerId = atoi(tempp);
			printf("Peer %d connected\n", peerId);
			
			//get ip port to update status
			tempp = strtok(NULL, " ");
			char *ip = strdup(tempp);
  			tempp = strtok(NULL, " ");
   			char *port = strdup(tempp);
   			port = strtok(port, "\n");
   			port = strtok(port, "\r");
			
			//call status update fuction
			pthread_mutex_lock(&lock1);
			peerStatusUpdate(ip, port, 1);
			bzero(client_message, MESSAGE_SIZE);
			bzero(my_message, MESSAGE_SIZE);
			bzero(file_message, MESSAGE_SIZE);
			pthread_mutex_unlock(&lock1);
		}
		
		//create tracker message from peer
		else if(strcmp(tempp, "create") == 0) {
		
			printf("Peer %d: %s",peerId,client_message);
			
			//getting file name
			tempp = strtok(NULL, " ");
			
			//creating tracker file
			char *token;
			char temp[100];
			token = strtok(tempp, ".");
			sprintf(token, "%s.track",token);
			FILE *fp;
			
			//checking if tracker file already exists or not
			if( access( token, F_OK ) == 0 ) {
				printf("File already exists\n");
    				sprintf(temp,"<createtracker ferr>");
    				
			} else {
			
				//create new tracker file if does not exist
    				fp = fopen(token,"w");
    				char tracker[MESSAGE_SIZE];
    				strncpy(tracker,&client_message[7],MESSAGE_SIZE);
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
   					
   					//writing these information in tracker file
   					fprintf(fp,"%s %s %s %s\n",fileName,fileSize,md5,desc);
   					fprintf(fp,"%s %s\n0 %s\n1\n",ip,port,fileSize);
   					fclose(fp);
   					
   					//entry of this tracker file in list file
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
			bzero(client_message, MESSAGE_SIZE);
			bzero(my_message, MESSAGE_SIZE);
			bzero(file_message, MESSAGE_SIZE);		
		}
		
		//update tracker request from peer
		else if(strcmp(tempp, "update") ==0) {
			
			printf("Peer %d: %s",peerId,client_message);
			
			tempp = strtok(NULL, " ");
			char *token;
			char temp[100];
			token = strtok(tempp, ".");
			sprintf(token, "%s.track",token);
			FILE *fp;
			if( access( token, F_OK ) == 0 ) {
			
				pthread_mutex_lock(&lock1);
				fp = fopen(token,"a+");
				pthread_mutex_unlock(&lock1);
				
				char tracker[MESSAGE_SIZE];
    				strncpy(tracker,&client_message[7],MESSAGE_SIZE);
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
   					
   					//already exisiting ip port check
   					int lineCount = getLine(fp, ip, port);
   					if (lineCount == 0) {
   						pthread_mutex_lock(&lock1);
   						fp = fopen(token,"a");
   						fprintf(fp,"%s %s\n%s %s\n1\n",ip,port, start_bytes,end_bytes);
   						fclose(fp);
   						pthread_mutex_unlock(&lock1);
   					}
   					else {
   						pthread_mutex_lock(&lock1);
   						fp = fopen(token,"r");
   						replaceLine(fp, token, lineCount+1, start_bytes, end_bytes);
   						pthread_mutex_unlock(&lock1);
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
			
			pthread_mutex_lock(&lock5);
			write(sock , my_message , strlen(my_message));
			bzero(client_message, MESSAGE_SIZE);
			bzero(my_message, MESSAGE_SIZE);
			bzero(file_message, MESSAGE_SIZE);
			pthread_mutex_unlock(&lock5);
		}
		else if(strcmp(tempp, "req") ==0) {
			//hanlde request from peer
			printf("Peer %d: %s",peerId,client_message);
			
			FILE *fp;
			char *filename = "list";
			pthread_mutex_lock(&lock2);
			fp = fopen(filename, "r");
			char data[MESSAGE_SIZE];
			char *begin = "<REP LIST X>\n";
			strcat(my_message,begin);
			while(fgets(data, MESSAGE_SIZE, fp) != NULL) {
				strcat(my_message,data);
			}
			char *end = "<REP LIST END>\n";
			strcat(my_message,end);
			write(sock , my_message , strlen(my_message));
			pthread_mutex_unlock(&lock2);
			//puts("List sent!!");
			bzero(client_message, MESSAGE_SIZE);
			bzero(my_message, MESSAGE_SIZE);
			bzero(file_message, MESSAGE_SIZE);
		}
		else if(strcmp(tempp, "get") ==0) {
		
			printf("Peer %d: %s",peerId,client_message);
			tempp = strtok(NULL, " ");
			//tempp = strtok(tempp, "\n");
			FILE *fp;
			sprintf(tempp, "%s",tempp);
			pthread_mutex_lock(&lock1);
			if( access( tempp, F_OK ) == 0 ) {
				fp = fopen(tempp, "r");
				
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
  				
  				close(sock);
  				fclose(fp);
  				pthread_mutex_unlock(&lock1);
			} else{
				printf("failed to open file\n");
				close(sock);	
			}
			
		}
		else if(strcmp(tempp, "terminate") ==0) {
		
			printf("Peer %d: %s",peerId,client_message);
		
			tempp = strtok(NULL, " ");
			char *ip = strdup(tempp);
  			tempp = strtok(NULL, " ");
   			char *port = strdup(tempp);
   			port = strtok(port, "\n");
   			port = strtok(port, "\r");
			pthread_mutex_lock(&lock1);
			peerStatusUpdate(ip, port, 0);
			pthread_mutex_unlock(&lock1);
			close(sock);
		}
		
	}
	
	if(read_size == 0)
	{
		//printf("Peer %d terminated\n", peerId);
		fflush(stdout);
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}

int getLine(FILE * fp, char *ip, char *port) 
{
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

void replaceLine(FILE * fp, char *fileName, int lineNo, char *start, char *end) 
{
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
    	//printf("%d\n",lineNo);
    	FILE * nfp = fopen("temp.txt","w");
	while((read = getline(&line, &len, fp)) != -1) {
		//puts(line);
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

void replaceStatus(FILE * fp, char *fileName, int lineNo, int status) 
{
	//puts("in replace status");
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
    	//printf("%d\n",lineNo);
    	FILE * nfp = fopen("temp.txt","w");
	while((read = getline(&line, &len, fp)) != -1) {
		//puts(line);
		if(i==lineNo) {
			fprintf(nfp, "%d\n", status);
			//printf("replaced line no %d with %d\n", i, status);
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

static int parse_ext(const struct dirent *dir)
{
     if(!dir)
       return 0;

     if(dir->d_type == DT_REG) { /* only deal with regular file */
         const char *ext = strrchr(dir->d_name,'.');
         if((!ext) || (ext == dir->d_name))
           return 0;
         else {
           if(strcmp(ext, ".track") == 0)
             return 1;
         }
     }

     return 0;
}

void peerStatusUpdate(char *ip, char *port, int status)
{
	//get all tracker files
	struct dirent **namelist;
       int n;

       n = scandir(".", &namelist, parse_ext, alphasort);
       if (n < 0) {
          	perror("Scandir");
       }
       else {
       	while (n--) {
               	FILE *fp;
               	fp = fopen(namelist[n]->d_name,"r");
               			
               	//printf("opening %s\n",namelist[n]->d_name);
               	int lineCount = getLine(fp, ip, port);
               	if(lineCount!=0) {
   				fp = fopen(namelist[n]->d_name,"r");
   				replaceStatus(fp, namelist[n]->d_name, lineCount+2, status);
   			}
               			
               	free(namelist[n]);
           	}
           	free(namelist);
       }
}
 

