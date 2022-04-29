#include<stdio.h>
#include<stdlib.h>
#include<string.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include<pthread.h>
#include <time.h>

#define SIZE 1024
#define MESSAGE_SIZE 512
#define CHAR_SIZE 128

int peerId, flag=1;
pthread_mutex_t lock1, lock2, lock3, lock4, lock5;

struct ipPort{
	char ip[CHAR_SIZE];
	char port[CHAR_SIZE];
};

typedef struct peerInfo {
	char ip[CHAR_SIZE];
	char port[CHAR_SIZE];
	int socket;
	long start, end;
	int status;
} peerInfo;

/*This function returns filesize after
 getting file name as argument*/
off_t fsize(const char *filename);

/*This function is responsible
 for calculating md5. It uses openssl library*/
char * calculate_file_md5(const char *filename);

/*This function takes no argument.
 It is responsible for returning
 the ip address and port of peer server.
 It gets these information from config file*/
char * getConf();

/*This function returns the ip addess of peer server.
 It uses getConf() function first, then use
 string tokenizer*/
char * getIP();

/*This function returns the port of peer server.
 It uses getConf(), then use tokenizer, and
 atoi*/
int getPort();

/*This function returns the peer id from config
 file*/
int getID();

/*This function returns the tracker server's ip
 address by reading the config file*/
char * getServerIP();

/*This function returns the port of tracker server
 by reading the config file*/
int getServerPort();

/*This function returns the socket after successfully
 connectiong with any server. The server's ip address and port is given
 as argument to this function*/
int getClientSocket(struct ipPort ip_ports);

/*This function takes tracker file name as argument,
 and handle connection to all available peers,
 requesting chunks from them, writing to file,
 chceking md5 after download is compelted*/
void *downloadHandler(void *trackerName);

/*This function is responsible for all client communication
 with tracker server. It handles all basic commands
 like create, req, get from user input and send
 message to the tracker server*/
void clientThread(void *ip_port);

/*This function is responsible for peer's multi threaded
 server functionalities. This function mainly handles
 multi threaded download capabilties of peer*/
void *connection_handler(void *socket_desc);

/*This is the peer server function that is listening for
 incoming connections, and after successfully accepting a
 connection it assigns a connection handler
 mentioned above by multi threading*/
void *peerServer(void *unused);


int main(int argc , char *argv[])
{
	
	peerId = getID();
	
    /*Here we create our peer server that keeps
     listening for incoming connections*/
	pthread_t peer_server;
	if(pthread_create(&peer_server, NULL, peerServer, NULL) < 0) {
    		perror("Could not create server send thread");
    		return -1;
  	}
  	
  	char input[MESSAGE_SIZE], file_message[MESSAGE_SIZE];
  	
  	bzero(input, MESSAGE_SIZE);
	bzero(file_message, MESSAGE_SIZE);
	
	//conenct to tracker server
	struct ipPort *ip_port = malloc(sizeof(struct ipPort));
	sprintf(ip_port->ip,"%s", getServerIP());
	sprintf(ip_port->port,"%d", getServerPort());
	clientThread((void *) ip_port);
	
  	while(fgets(input, MESSAGE_SIZE, stdin) != NULL){
  		strcat(file_message, input);
		char *tempp;
		tempp = strtok(file_message, " ");
		//puts(tempp);
		
		//reconnect to tracker
		if(strcmp(tempp, "connect") == 10) {
			flag = 1;
			clientThread((void *) ip_port);
		}
		if(strcmp(tempp, "quit") == 10) {
			exit(0);
		}
		bzero(input, MESSAGE_SIZE);
		bzero(file_message, MESSAGE_SIZE);
  	}
	
	
	return 0;
}

void *peerServer(void *unused){
	int socket_desc , new_socket , c , *new_sock;
	struct sockaddr_in server , client;
	char *message;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr( getIP() ); 
	server.sin_port = htons( getPort() );
	
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
	char client_message[MESSAGE_SIZE], my_message[MESSAGE_SIZE], file_message[MESSAGE_SIZE];
	bzero(client_message, MESSAGE_SIZE);
	
	//Receive id from client
	int id;
	read_size = recv(sock , client_message , MESSAGE_SIZE , 0);
	id = atoi(client_message);
	
	bzero(client_message, MESSAGE_SIZE);
	bzero(my_message, MESSAGE_SIZE);
	bzero(file_message, MESSAGE_SIZE);
	
	while( (read_size = recv(sock , client_message , MESSAGE_SIZE , 0)) > 0 )
	{
		pthread_mutex_lock(&lock4);
		strcat(file_message, client_message);
		char *tempp;
		tempp = strtok(file_message, " ");
		
		//handle get request from other peers
		if(strcmp(tempp, "get") == 0) {
			
			tempp = strtok(NULL, " ");
			char *fileName = strdup(tempp);
			tempp = strtok(NULL, " ");
			char *start = strdup(tempp);
			tempp = strtok(NULL, " ");
			char *end = strdup(tempp);
			
			long start_t = atol(start);
			long end_t = atol(end);
			
			printf("Peer %d requested from %ld to %ld bytes\n",id,start_t,end_t);
			
			FILE *fp;
			
			//pthread_mutex_lock(&lock1);
			fp = fopen(fileName, "r");
			//pthread_mutex_unlock(&lock1);
			if (fp == NULL) {
				perror("File open failed");
			}
  			char data[SIZE] = {0};
  			long i = 0;
  			int bs;
  			while(1){
  				//pthread_mutex_lock(&lock5);
  				bs = fread(data, 1, sizeof(data), fp);
  				//pthread_mutex_lock(&lock5);
				if(i==start_t){
					//pthread_mutex_lock(&lock2);
					write(sock, data, bs);
					//pthread_mutex_unlock(&lock2);
					break;
				}	
    				bzero(data, SIZE);
    				i+=SIZE;
    				//pthread_mutex_lock(&lock3);
    				if(feof(fp)) {
    					break;
    				}
    				//pthread_mutex_unlock(&lock3);
  			}
  			
  			fclose(fp);
			bzero(client_message, MESSAGE_SIZE);
			bzero(my_message, MESSAGE_SIZE);
			bzero(file_message, MESSAGE_SIZE);
			free(fileName);
			free(start);
			free(end);
			
		}
		pthread_mutex_unlock(&lock4);
	}
	
	if(read_size == 0)
	{
		printf("Peer %d disconnected\n", id);
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		printf("Peer %d disconnected\n", id);
		//perror("Connection terminated");
	}
		
	//Free the socket pointer
	free(socket_desc);
	
	return 0;
}

//function to handle communication with tracker_server and other peers as client
void clientThread(void *ip_port)
{
	
	struct ipPort ip_ports = *(struct ipPort*)ip_port;
	int socket_desc = getClientSocket(ip_ports);
	//struct sockaddr_in server;
	
	char *message , server_reply[MESSAGE_SIZE], my_reply[MESSAGE_SIZE], input[MESSAGE_SIZE],file_message[MESSAGE_SIZE];
	int read_size;
	
	bzero(my_reply, MESSAGE_SIZE);
	bzero(server_reply, MESSAGE_SIZE);
	bzero(input, MESSAGE_SIZE);
	bzero(file_message, MESSAGE_SIZE);
	
	sprintf(my_reply,"intro %d %s", peerId, getConf());
	if(write(socket_desc , my_reply , strlen(my_reply))<0){
		puts("send failed!!");
	}
	
	bzero(my_reply, MESSAGE_SIZE);
	while(fgets(input, MESSAGE_SIZE, stdin) != NULL) {
		strcat(file_message, input);
		char *tempp;
		tempp = strtok(file_message, " ");
		//puts(tempp);
		if(strcmp(tempp, "create") == 0) {
			puts("Create from client");
			
			//get file name
			tempp = strtok(NULL, " ");
			char fileName[CHAR_SIZE];
			sprintf(fileName,"%s", tempp);
			
			//give description as description
			tempp = strtok(NULL, " ");
			tempp = strtok(tempp,"\n");
			char fileDesc[CHAR_SIZE];
			sprintf(fileDesc,"%s", tempp);
			
			//final message
			sprintf(my_reply,"create %s %ld %s %s %s",fileName, (long int) fsize(fileName), fileDesc, calculate_file_md5(fileName),getConf());
			
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , MESSAGE_SIZE , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "update") ==0){
			if(write(socket_desc, input , strlen(input))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , MESSAGE_SIZE , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "req") ==0){
			if(write(socket_desc , input , strlen(input))<0){
				puts("send failed!!");
			}
			recv(socket_desc , server_reply , MESSAGE_SIZE , 0);
			puts(server_reply);
		}
		else if(strcmp(tempp, "get") ==0){
			
			tempp = strtok(NULL, " ");
			
			int n;
  			FILE *fp;
  			char *filename = strdup(tempp);
  			char buffer[SIZE];
  			fp = fopen(filename, "w");
			int i=0;
			if(write(socket_desc , input , strlen(input))<0){
				puts("get failed!!");
			}
			bzero(buffer, SIZE);
  			while (1) {
    				n = read(socket_desc, buffer, SIZE);
    				if (n <= 0){
      					break;
    				}
    				fwrite(buffer, 1, n, fp);
    				bzero(buffer, SIZE);
    				i++;
  			}
  			fclose(fp);
  			
  			pthread_t peer_download;
			if(pthread_create(&peer_download, NULL, downloadHandler, filename) < 0) {
    				perror("Could not create server send thread");
    				return;
  			}
  			
  			printf("\n");
  			char end[CHAR_SIZE];
  			sprintf(end,"%s received",filename);
  			puts(end);
  			
  			//reconnect to tracker
  			socket_desc = getClientSocket(ip_ports);
  			sprintf(my_reply,"intro %d %s", peerId, getConf());
			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
  		}
  		
  		else if(strcmp(tempp, "terminate") == 10) {
  			//construct terminate message
  			sprintf(my_reply,"terminate %s", getConf());
  			
  			if(write(socket_desc , my_reply , strlen(my_reply))<0){
				puts("send failed!!");
			}
			
  			close(socket_desc);
			return;
		}
  		
  		bzero(my_reply, MESSAGE_SIZE);
		bzero(server_reply, MESSAGE_SIZE);
		bzero(input, MESSAGE_SIZE);
		bzero(file_message, MESSAGE_SIZE);
	}
	return;
}

void *downloadHandler(void *trackerName)
{
	peerInfo peerList[CHAR_SIZE];
	int listSize = 0;
	char* fileName;
	long fileSize;
	char* md5;
	char *trackerFileName = strdup((char *) trackerName);
	int socket_desc;
	
	struct ipPort ip_ports2;
	sprintf(ip_ports2.ip,"%s",getServerIP());
	sprintf(ip_ports2.port,"%d",getServerPort());
	int tracker_sock = getClientSocket(ip_ports2);
	
	char my_reply[MESSAGE_SIZE];
	bzero(my_reply, MESSAGE_SIZE);
	
	sprintf(my_reply,"intro %d %s", peerId, getConf());
	if(write(tracker_sock , my_reply , strlen(my_reply))<0){
		puts("send failed!!");
	}
	bzero(my_reply, MESSAGE_SIZE);
	
	//need to handle already downloaded part
	
	//getting peer info from tracker file
	FILE *fp = fopen(trackerFileName, "r");
	char *line;
	size_t len = 0;
    	ssize_t read;
    	int lineCount = 0;
    	int disCount = 0; //number of disconnected peer
    	
	while((read = getline(&line, &len, fp)) != -1) {
		//getting filename, size, md5 from first line
		if(lineCount == 0) {
			char *token = strtok(line, " ");
			fileName = strdup(token);
			token = strtok(NULL, " ");
			fileSize = atol(token);
			token = strtok(NULL, " ");
			md5 = strdup(token);
			
		}
		else if(lineCount%3 == 1) {
			char *token = strtok(line, " ");
			bzero(peerList[listSize].ip, CHAR_SIZE);
			sprintf(peerList[listSize].ip,"%s",token);
			token = strtok(NULL, " ");
			bzero(peerList[listSize].port, CHAR_SIZE);
			sprintf(peerList[listSize].port,"%s",token);
		}
		else if(lineCount%3 == 2) {
			char *token = strtok(line, " ");
			peerList[listSize].start = atol(token);
			token = strtok(NULL, " ");
			peerList[listSize].end = atol(token);
		}
		//set online or offline status
		else if(lineCount%3 == 0) {
			peerList[listSize].status = atoi(line);
			if(peerList[listSize].status == 1){
				
				struct ipPort ip_ports;
				sprintf(ip_ports.ip,"%s",peerList[listSize].ip);
				sprintf(ip_ports.port,"%s",peerList[listSize].port);
				
				//connect to online peers
				int socket = getClientSocket(ip_ports);
				peerList[listSize].socket = socket;
				
				sprintf(my_reply,"%d", peerId);
				if(write(socket , my_reply , strlen(my_reply))<0){
					puts("send failed!!");
				}
				bzero(my_reply, MESSAGE_SIZE);
				
				//online peers get added to the list
				listSize++; 
			}
		}
		lineCount++;
	}
	
	fclose(fp);
	
	//sort peers according to their start byte
	for(int j=0;j<listSize;j++){
		for(int k=j+1;k<listSize;k++){
			peerInfo temp;
			if(peerList[k].start < peerList[j].start) {
				temp = peerList[j];
				peerList[j] = peerList[k];
				peerList[k] = temp;
			}
		}
	}
	
	//printf("list size %d\n",listSize);
	//printf("%s %s %ld %ld\n", peerList[0].ip, peerList[0].port, peerList[0].start, peerList[0].end);
	
	char buffer[SIZE];
	bzero(my_reply, MESSAGE_SIZE);
	bzero(buffer, SIZE);
	long rcvBytes = 0;
	int currPeer=0;
	FILE *nfp = fopen(fileName,"w");
	int c = 0;
	sleep(1);
	
	//receiving files
	while(rcvBytes <= fileSize && listSize>0) {
		
		long start_t = rcvBytes;
		long end_t = start_t+SIZE;
		if(end_t > fileSize) {
			end_t = fileSize;
		}
		
		struct ipPort ip_ports;
		
		//find ip port suitable for next download
		if(peerList[currPeer].start > start_t) {
			int t = currPeer-1;
			while(t>=0) {
				if(peerList[t].start <= start_t){
					currPeer = t;
					break;
				}
				t--;
			}
		}
		
		//check if endl is available in peer server
		if(peerList[currPeer].end < end_t) {
			int t = 0;
			while(t<listSize) {
				if(peerList[t].end >= end_t && peerList[t].start <= start_t){
					currPeer = t;
					break;
				}
				t++;
			}
		}
		
		sprintf(ip_ports.ip,"%s",peerList[currPeer].ip);
		sprintf(ip_ports.port,"%s",peerList[currPeer].port);
		int socket_desc = peerList[currPeer].socket;
		
		sprintf(my_reply,"get %s %ld %ld",fileName, start_t, end_t);
		
		//send get request for specific bytes
		if(write(socket_desc , my_reply, strlen(my_reply))<0){
			puts("get failed!!");
		}
		
		int n;
		n = recv(socket_desc, buffer, SIZE, 0);
		
		//if receive was bad
    		if (n <= 0){
    			puts("Retry");
    			bzero(my_reply, MESSAGE_SIZE);
			bzero(buffer, SIZE);
			currPeer++;
			if(currPeer == listSize){
				currPeer = 0;
			}
      			continue;
    		}
		
		//write to file
		fwrite(buffer, 1, n, nfp);
		bzero(my_reply, MESSAGE_SIZE);
		bzero(buffer, SIZE);
		
		//check if every chunk was downloaded
		if(end_t == fileSize){
			fclose(nfp);
			
			//check md5
			if(strcmp(calculate_file_md5(fileName),md5) == 0){
				puts("md5 check done. Received Correctly");
			}
			else{
				//printf("md5: %s\nreceived md5: %s\n",md5, calculate_file_md5(fileName));
				puts("md5 check done. Not received Correctly");
			}
			
			for(int j=0;j<listSize;j++){
				close(peerList[j].socket);
			}
			printf("Download completed for %s\n",fileName);
			
			sprintf(my_reply,"update %s 0 %ld %s",fileName, end_t, getConf());
			if(write(tracker_sock, my_reply, strlen(my_reply))<0){
				puts("update failed!!");
			}
			close(tracker_sock);
			
			return 0;
		}
		rcvBytes = end_t;
		currPeer++;
		
		if(currPeer == listSize){
			currPeer = 0;
		}
		
		
  		//send update to tracker
  		if( c == MESSAGE_SIZE) {
			sprintf(my_reply,"update %s 0 %ld %s",fileName, end_t, getConf());
			if(write(tracker_sock, my_reply, strlen(my_reply))<0){
				puts("Update failed!!");
			}
			
			c=-1;
  		}
  		c++;
	}
}


off_t fsize(const char *filename) 
{
    struct stat st; 

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1; 
}

char * calculate_file_md5(const char *filename) 
{
	unsigned char c[MD5_DIGEST_LENGTH];
	int i;
	MD5_CTX mdContext;
	int bytes;
	unsigned char data[SIZE];
	char *filemd5 = (char*) malloc(33 *sizeof(char));

	FILE *inFile = fopen (filename, "rb");
	if (inFile == NULL) {
		perror(filename);
		return 0;
	}

	MD5_Init (&mdContext);

	while ((bytes = fread (data, 1, SIZE, inFile)) != 0)

	MD5_Update (&mdContext, data, bytes);

	MD5_Final (c,&mdContext);

	for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
		sprintf(&filemd5[i*2], "%02x", (unsigned int)c[i]);
	}

	fclose (inFile);
	return filemd5;
}

char * getConf()
{
	FILE * fp;
	fp = fopen("client.config", "r");
	char *conf;
	char *result = (char*) malloc(CHAR_SIZE *sizeof(char));
    size_t len = 0;
    ssize_t read;
	read = getline(&conf, &len, fp);
	sprintf(result,"%s",conf);
	return result;
}

char * getIP()
{
	char *ip_port = getConf();
	char *token = strtok(ip_port," ");
	return token;
}

int getPort()
{
	char *ip_port = getConf();
	char *token = strtok(ip_port," ");
	token = strtok(NULL," ");
	//puts(token);
	int port = atoi(token);
	return port;
}

int getID()
{
	FILE * fp;
	fp = fopen("client.config", "r");
	char *conf;
	char *result = (char*) malloc(CHAR_SIZE *sizeof(char));
    	size_t len = 0;
    	ssize_t read;
	read = getline(&conf, &len, fp);
	read = getline(&conf, &len, fp);
	sprintf(result,"%s",conf);
	return atoi(result);
}

char * getServerIP()
{
	FILE * fp;
	fp = fopen("client.config", "r");
	char *conf;
	char *result = (char*) malloc(CHAR_SIZE *sizeof(char));
    	size_t len = 0;
    	ssize_t read;
	read = getline(&conf, &len, fp);
	read = getline(&conf, &len, fp);
	read = getline(&conf, &len, fp);
	char *token = strtok(conf," ");
	return token;
}

int getServerPort()
{
	FILE * fp;
	fp = fopen("client.config", "r");
	char *conf;
	char *result = (char*) malloc(CHAR_SIZE *sizeof(char));
    	size_t len = 0;
    	ssize_t read;
	read = getline(&conf, &len, fp);
	read = getline(&conf, &len, fp);
	read = getline(&conf, &len, fp);
	char *token = strtok(conf," ");
	token = strtok(NULL," ");
	return atoi(token);
}

int getClientSocket(struct ipPort ip_ports) 
{
	struct sockaddr_in server;
	int socket_desc;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
    
	server.sin_addr.s_addr = inet_addr(ip_ports.ip);
	server.sin_family = AF_INET;
	server.sin_port = htons( atoi(ip_ports.port) );
	
	//Connect to remote server
	if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("Connect error");
		//exit(0);
	}
	if(flag == 1)
		puts("Connected to tracker");
		
	flag = 0;
	
	return socket_desc;
}
