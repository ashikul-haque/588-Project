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


/*This function is responsible for tracker's multi-threaded
 server functionalities. This function mainly handles
 multi-threaded incoming requests of peers.*/
void *connection_handler(void *);

/*This is the tracker server function that is listening for
 incoming connections, and after successfully accepting a
 connection it assigns a connection handler
 mentioned above by multi-threading*/
void *trackerServer(void *);

/*This function is a utility function. It takes file
 pointer, ip, port as argument and returns the line no that
 contains this ip and port*/
int getLine(FILE * fp, char *ip, char *port);

/*This function is a utility function. It takes file pointer,
 file name, line no to replcae, start, end as argument.
 It replace the specific line of a file with start and end.
 This function mainly helps with updating exisiting peers'
 downloaded number of chunks*/
void replaceLine(FILE * fp, char *fileName, int lineNo, char *start, char *end);

/*This function is a utility function. It takes file pointer,
 file name, line no to replcae, status as argument.
 It replace the specific line of a file with status.
 This function mainly helps with updating exisiting peers'
 online or offline status*/
void replaceStatus(FILE * fp, char *fileName, int lineNo, int status);

/*This function is also a utility function. It helps with
 finding all the files with a extension in a directory. It
 enables us to update all the track file with above
 mentioned replaceStatus function*/
static int parse_ext(const struct dirent *dir);

/*This function utilizes above mentioned functions
 replaceStatus and parse_ext*/
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
	
	//Listen
	listen(socket_desc , 3);
	
	//Accept incoming connection and assigns a handler for each of them
    //handler is multi-threaded function
	c = sizeof(struct sockaddr_in);
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
		
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = new_socket;
		
        //using threading to enable multi user communication
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
		{
			perror("could not create thread");
			exit(1);
		}

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
            
            //trimming \n and \r
   			port = strtok(port, "\n");
   			port = strtok(port, "\r");
			
			//call status update fuction
            //using mutex lock because file is get edited here
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
            
            //get only file name withour extension
            //add .track to it
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
                    //file name
   					char *fileName = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					
                    //file size
   					char *fileSize = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					
                    //file description
   					char *desc = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					
                    //md5
   					char *md5 = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					
                    //ip address
   					char *ip = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
   					
                    //port
   					char *port = strdup(tracker_tok);
   					port = strtok(port, "\n");
   					port = strtok(port, "\r");
   					
   					//writing these information in tracker file
   					fprintf(fp,"%s %s %s %s\n",fileName,fileSize,md5,desc);
   					fprintf(fp,"%s %s\n0 %s\n1\n",ip,port,fileSize);
   					fclose(fp);
   					
   					//entry of this tracker file in list file
                    //it will help us when peer send req message
   					FILE *listFP;
   					char *listName = "list";
   					if (access( token, F_OK ) == 0 ) {
                        //if list file already exists, we just append to it
						listFP = fopen(listName, "a");
						fprintf(listFP,"%s %s %s\n",fileName,fileSize,md5);
					} 
					else {
						//if list file does not exist already we open a new file
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
            //sending reply to create request
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
                //locking because multiple thread might want to open file at the same time
				pthread_mutex_lock(&lock1);
				fp = fopen(token,"a+");
				pthread_mutex_unlock(&lock1);
				
				char tracker[MESSAGE_SIZE];
    				strncpy(tracker,&client_message[7],MESSAGE_SIZE);
    				if ( fp )
   				{
                    //getting all the information from messsage
   					char *tracker_tok = strtok(tracker, " ");
   					tracker_tok = strtok(NULL, " ");
                    
                    //start byte
   					char *start_bytes = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
                    
                    //end byte
   					char *end_bytes = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
                    
                    //ip address of peer
   					char *ip = strdup(tracker_tok);
   					tracker_tok = strtok(NULL, " ");
                    
                    //port of peer
   					char *port = strdup(tracker_tok);
                    
                    //trimming
   					port = strtok(port, "\n");
   					port = strtok(port, "\r");
   					
   					//already exisiting ip port check
                    //if we get 0, the peer does not exist yet
   					int lineCount = getLine(fp, ip, port);
   					if (lineCount == 0) {
                        //locking because we write new information to the file
   						pthread_mutex_lock(&lock1);
   						fp = fopen(token,"a");
                        //appending new peer information
   						fprintf(fp,"%s %s\n%s %s\n1\n",ip,port, start_bytes,end_bytes);
   						fclose(fp);
   						pthread_mutex_unlock(&lock1);
   					}
   					else {
                        //locking because we write new information to the file
   						pthread_mutex_lock(&lock1);
   						fp = fopen(token,"r");
                        //we update existing information
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
            //sends update reply
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
            //just access the already written file list
			pthread_mutex_lock(&lock2);
			fp = fopen(filename, "r");
			char data[MESSAGE_SIZE];
			char *begin = "<REP LIST X>\n";
			strcat(my_message,begin);
            
            //read line by line from list
            //create message appending lines from file
			while(fgets(data, MESSAGE_SIZE, fp) != NULL) {
				strcat(my_message,data);
			}
			char *end = "<REP LIST END>\n";
			strcat(my_message,end);
            
            //sends the contents of this file as message that is
            //kept in my_message
			write(sock , my_message , strlen(my_message));
			pthread_mutex_unlock(&lock2);
			bzero(client_message, MESSAGE_SIZE);
			bzero(my_message, MESSAGE_SIZE);
			bzero(file_message, MESSAGE_SIZE);
		}
		else if(strcmp(tempp, "get") ==0) {
            
            //handle get track file request of peer
			printf("Peer %d: %s",peerId,client_message);
			tempp = strtok(NULL, " ");
			
			FILE *fp;
			sprintf(tempp, "%s",tempp);
            
			pthread_mutex_lock(&lock1);
            
            //checks if the tracker file exists or not
			if( access( tempp, F_OK ) == 0 ) {
				fp = fopen(tempp, "r");
				
  				char data[SIZE];
  				int i = 0;
  				bzero(data, SIZE);
  				int bs;
  				
                //read the file by 1024 bytes
  				while(!feof(fp)) {
  					bs = fread(data, 1, sizeof(data), fp);
                    
                    //sends the data that was read
					write(sock, data, bs);
    					bzero(data, SIZE);
    					i+=SIZE;	
  				}
  				
                //close socket
  				close(sock);
                //close file
  				fclose(fp);
  				pthread_mutex_unlock(&lock1);
			} else{
				printf("failed to open file\n");
				close(sock);	
			}
			
		}
		else if(strcmp(tempp, "terminate") ==0) {
            //terminate message from peer, when it wants to disconnect
			printf("Peer %d: %s",peerId,client_message);
		
            //gets the ip and port the peer that wants to disconnect
			tempp = strtok(NULL, " ");
			char *ip = strdup(tempp);
  			tempp = strtok(NULL, " ");
   			char *port = strdup(tempp);
            
            //trim
   			port = strtok(port, "\n");
   			port = strtok(port, "\r");
            
            //locking file before editing it
			pthread_mutex_lock(&lock1);
            //update the connection status of disconnected peer
			peerStatusUpdate(ip, port, 0);
			pthread_mutex_unlock(&lock1);
            //disconnect
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
    
    //read the file line by line
	while((read = getline(&line, &len, fp)) != -1) {
		
		char ip_port[100];
		sprintf(ip_port,"%s %s\n",ip, port); 
		
        //checks if the line matches with given ip port
		if(strcmp(ip_port, line) == 0) {
            //if matches, return the number of line
			return i;
		}
		bzero(line, read);
		i++;
	}
	fclose(fp);
    //if not found, return 0
	return 0;
}

void replaceLine(FILE * fp, char *fileName, int lineNo, char *start, char *end) 
{
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
    //use a new file for editing
    	FILE * nfp = fopen("temp.txt","w");
	while((read = getline(&line, &len, fp)) != -1) {
		//puts(line);
        //naviagte to the line no given as input
		if(i==lineNo) {
            //edit that line
			fprintf(nfp, "%s %s\n", start, end);
		}
		else {
            //other lines remain same. just copy them to new file
			fprintf(nfp, "%s", line);
		}
		i++;
	}
    //close both files
	fclose(fp);
	fclose(nfp);
    //remove already existing file
	remove(fileName);
    //rename the new file to the previous one
    	rename("temp.txt",fileName);
}

void replaceStatus(FILE * fp, char *fileName, int lineNo, int status) 
{
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int i = 1;
    //use a new file for editing
    	FILE * nfp = fopen("temp.txt","w");
	while((read = getline(&line, &len, fp)) != -1) {
		//naviagte to the line no given as input
        if(i==lineNo) {
            //edit this line
			fprintf(nfp, "%d\n", status);
		}
		else {
            //other line remain same. just copy them to new file
			fprintf(nfp, "%s", line);
		}
		i++;
	}
	fclose(fp);
	fclose(nfp);
    //remove already exisitng file
	remove(fileName);
    //rename to the previous one
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
             //only get .track files
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
       //use parse_ext to get only the .track files
       n = scandir(".", &namelist, parse_ext, alphasort);
       if (n < 0) {
          	perror("Scandir");
       }
       else {
        //iterate through all track files
       	while (n--) {
               	FILE *fp;
               	fp = fopen(namelist[n]->d_name,"r");
                //checks if this track file has the given ip and port
               	int lineCount = getLine(fp, ip, port);
                //if found, call replace status to update the status of that peer
               	if(lineCount!=0) {
   				fp = fopen(namelist[n]->d_name,"r");
   				replaceStatus(fp, namelist[n]->d_name, lineCount+2, status);
   			}
               			
               	free(namelist[n]);
           	}
           	free(namelist);
       }
}
 

